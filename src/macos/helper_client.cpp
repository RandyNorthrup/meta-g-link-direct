// SPDX-License-Identifier: AGPL-3.0-only
#include "helper_client.hpp"

#include "mgl/identity.hpp"

#include <poll.h>
#include <spawn.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>

extern char** environ; // NOLINT(readability-redundant-declaration) — POSIX declares it nowhere.

namespace mgl::helper {
namespace {
// Includes the time a person takes to answer a permission question the first time.
constexpr auto HelperAnswerWait = std::chrono::seconds(90);

std::optional<std::string> helperApp() {
    const auto directory = identity::storageDirectory();
    if (!directory) {
        return std::nullopt;
    }
    std::string path = *directory + "/bin/Meta G Link Helper.app";
    struct stat info{};
    if (stat(path.c_str(), &info) != 0) {
        return std::nullopt;
    }
    return path;
}

// A path for a listening socket in a fresh private folder. Empty on failure. sockaddr_un limits the
// path, and the storage folder can be long, so it goes in the per-user temporary folder macOS
// provides.
std::string makeSocketPath() {
    const char* temporary = std::getenv("TMPDIR"); // NOLINT(concurrency-mt-unsafe)
    std::string pattern = std::string(temporary != nullptr ? temporary : "/tmp/") + "mgl.XXXXXX";
    if (mkdtemp(pattern.data()) == nullptr) {
        return {};
    }
    return pattern + "/s";
}

void removeSocket(const std::string& path) {
    (void)unlink(path.c_str());
    (void)rmdir(path.substr(0, path.rfind('/')).c_str());
}

bool launchHelper(const std::string& app, const char* role, const std::string& socket_path) {
    std::string open_tool = "/usr/bin/open";
    std::string new_instance = "-n";
    std::string background = "-g";
    std::string arguments_flag = "--args";
    std::string role_copy = role;
    std::string app_copy = app;
    std::string socket_copy = socket_path;
    std::array<char*, 8> arguments{
        open_tool.data(),      new_instance.data(), background.data(),  app_copy.data(),
        arguments_flag.data(), role_copy.data(),    socket_copy.data(), nullptr,
    };
    pid_t process = 0;
    if (posix_spawn(&process, open_tool.c_str(), nullptr, nullptr, arguments.data(), environ) !=
        0) {
        return false;
    }
    int status = 0;
    while (waitpid(process, &status, 0) < 0) {
        if (errno != EINTR) {
            return false;
        }
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool waitReadable(int descriptor, std::chrono::milliseconds wait) {
    pollfd readable{.fd = descriptor, .events = POLLIN, .revents = 0};
    return poll(&readable, 1, static_cast<int>(wait.count())) > 0;
}
} // namespace

Descriptor::~Descriptor() {
    if (value_ != None) {
        close(value_);
    }
}

Descriptor startHelper(const char* role, int& error) {
    const auto app = helperApp();
    if (!app) {
        error = ENOENT;
        return Descriptor(Descriptor::None);
    }
    const std::string path = makeSocketPath();
    sockaddr_un address{};
    if (path.empty() || path.size() >= sizeof(address.sun_path)) {
        error = ENAMETOOLONG;
        return Descriptor(Descriptor::None);
    }
    address.sun_family = AF_UNIX;
    std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
    const Descriptor listener(socket(AF_UNIX, SOCK_STREAM, 0));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) — the socket C ABI type-erases.
    if (listener.get() < 0 ||
        bind(listener.get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0 ||
        listen(listener.get(), 1) != 0) {
        error = errno;
        removeSocket(path);
        return Descriptor(Descriptor::None);
    }
    if (!launchHelper(*app, role, path) || !waitReadable(listener.get(), HelperAnswerWait)) {
        error = ETIMEDOUT;
        removeSocket(path);
        return Descriptor(Descriptor::None);
    }
    Descriptor peer(accept(listener.get(), nullptr, nullptr));
    const int accept_error = errno;
    removeSocket(path); // a connected socket no longer needs its name
    if (peer.get() < 0) {
        error = accept_error;
        return Descriptor(Descriptor::None);
    }
    const int on = 1;
    (void)setsockopt(peer.get(), SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
    return peer;
}

std::optional<Answer> readAnswer(int descriptor) {
    constexpr std::size_t LongestLine = 64;
    std::string line;
    const auto deadline = std::chrono::steady_clock::now() + HelperAnswerWait;
    // A byte at a time, so nothing after the line is consumed.
    for (;;) {
        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (line.size() >= LongestLine || left.count() <= 0 || !waitReadable(descriptor, left)) {
            return std::nullopt;
        }
        char byte = 0;
        if (read(descriptor, &byte, 1) <= 0) {
            return std::nullopt;
        }
        if (byte == '\n') {
            break;
        }
        line.push_back(byte);
    }
    const auto space = line.find(' ');
    Answer answer{.status = line.substr(0, space)};
    if (space != std::string::npos) {
        answer.detail = static_cast<int>(std::strtol(line.c_str() + space + 1, nullptr, 10));
    }
    return answer;
}

bool sendText(int descriptor, const std::string& text) {
    std::size_t sent = 0;
    while (sent < text.size()) {
        const auto wrote = send(descriptor, text.data() + sent, text.size() - sent, 0);
        if (wrote < 0 && errno == EINTR) {
            continue;
        }
        if (wrote <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(wrote);
    }
    return true;
}

void awaitEnd(int descriptor, std::chrono::milliseconds limit) {
    const auto deadline = std::chrono::steady_clock::now() + limit;
    for (;;) {
        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        char byte = 0;
        if (left.count() <= 0 || !waitReadable(descriptor, left) ||
            read(descriptor, &byte, 1) <= 0) {
            return;
        }
    }
}
} // namespace mgl::helper
