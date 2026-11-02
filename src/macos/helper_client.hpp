// SPDX-License-Identifier: AGPL-3.0-only
// The caller's side of Meta G Link Helper (helper_protocol.hpp): starts it for one role and reads
// its answer.
#ifndef MGL_MACOS_HELPER_CLIENT_HPP
#define MGL_MACOS_HELPER_CLIENT_HPP

#include <chrono>
#include <optional>
#include <string>

namespace mgl::helper {
class Descriptor {
  public:
    static constexpr int None = -1;

    explicit Descriptor(int value) : value_(value) {}
    Descriptor(const Descriptor&) = delete;
    Descriptor& operator=(const Descriptor&) = delete;
    Descriptor(Descriptor&& other) noexcept : value_(other.release()) {}
    Descriptor& operator=(Descriptor&&) = delete;
    ~Descriptor();
    [[nodiscard]] int get() const noexcept { return value_; }
    [[nodiscard]] bool valid() const noexcept { return value_ != None; }
    int release() noexcept {
        const int value = value_;
        value_ = None;
        return value;
    }

  private:
    int value_;
};

struct Answer {
    std::string status;
    int detail = 0;
};

// Starts the helper for one role through LaunchServices, so macOS treats it as its own app and not
// as a part of the caller, whose privacy settings it would otherwise share. Returns the connected
// socket, or an invalid descriptor with the cause in error. ENOENT means the helper isn't
// installed.
[[nodiscard]] Descriptor startHelper(const char* role, int& error);

// The helper's status line. Nullopt when it ended first, or did not answer in time, which includes
// a person answering a permission question the first time.
[[nodiscard]] std::optional<Answer> readAnswer(int descriptor);

// Writes the whole text. False when the helper has gone; never raises SIGPIPE in the host process.
[[nodiscard]] bool sendText(int descriptor, const std::string& text);

// Waits up to the limit for the helper to close its end.
void awaitEnd(int descriptor, std::chrono::milliseconds limit);
} // namespace mgl::helper
#endif
