// SPDX-License-Identifier: AGPL-3.0-only
#include "setup_process.hpp"

#include "mgl/identity.hpp"

#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <thread>

extern char** environ; // NOLINT(readability-redundant-declaration) — POSIX declares it nowhere.

namespace mgl::plugin {
namespace {
// How often a wait looks at the process and at a stop request.
constexpr auto PollInterval = std::chrono::milliseconds(200);

// The installed setup window under this project's folder, when present. On macOS it is the
// executable inside the app bundle, so it gets its own window and Dock entry.
std::optional<std::string> toolPath() {
    const auto directory = mgl::identity::storageDirectory();
    if (!directory) {
        return std::nullopt;
    }
#ifdef __APPLE__
    std::string path = *directory + "/bin/Meta G Link Setup.app/Contents/MacOS/meta-g-link-setup";
#else
    std::string path = *directory + "/bin/meta-g-link-setup";
#endif
    struct stat info{};
    if (stat(path.c_str(), &info) != 0) {
        return std::nullopt;
    }
    return path;
}
} // namespace

struct SetupProcess::Impl {
    pid_t process = -1;
    std::atomic<bool> stop{false};
};

SetupProcess::SetupProcess() : impl_(std::make_unique<Impl>()) {}
SetupProcess::~SetupProcess() = default;

SetupProcess::Start SetupProcess::start(std::uint32_t& error) {
    auto tool = toolPath();
    if (!tool) {
        return Start::ToolMissing;
    }
    std::array<char*, 2> arguments{tool->data(), nullptr};
    const int spawned =
        posix_spawn(&impl_->process, tool->c_str(), nullptr, nullptr, arguments.data(), environ);
    if (spawned != 0) {
        error = static_cast<std::uint32_t>(spawned);
        return Start::Failed;
    }
    return Start::Started;
}

std::optional<std::uint32_t> SetupProcess::wait() {
    constexpr std::uint32_t NoExitCode = 255;
    while (!impl_->stop.load()) {
        int status = 0;
        const pid_t done = waitpid(impl_->process, &status, WNOHANG);
        if (done == impl_->process) {
            return WIFEXITED(status) ? static_cast<std::uint32_t>(WEXITSTATUS(status)) : NoExitCode;
        }
        if (done < 0 && errno != EINTR) {
            return NoExitCode;
        }
        std::this_thread::sleep_for(PollInterval);
    }
    return std::nullopt;
}

void SetupProcess::requestStop() { impl_->stop.store(true); }
} // namespace mgl::plugin
