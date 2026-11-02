// SPDX-License-Identifier: AGPL-3.0-only
// Starts the installed setup window and waits for it to close.
#ifndef MGL_PLUGIN_SETUP_PROCESS_HPP
#define MGL_PLUGIN_SETUP_PROCESS_HPP

#include <cstdint>
#include <memory>
#include <optional>

namespace mgl::plugin {
class SetupProcess {
  public:
    enum class Start : std::uint8_t { Started, ToolMissing, Failed };

    SetupProcess();
    SetupProcess(const SetupProcess&) = delete;
    SetupProcess& operator=(const SetupProcess&) = delete;
    SetupProcess(SetupProcess&&) = delete;
    SetupProcess& operator=(SetupProcess&&) = delete;
    ~SetupProcess();

    // Starts the setup window. error holds the system's error code for Failed.
    [[nodiscard]] Start start(std::uint32_t& error);
    // Blocks until the window closes or requestStop is called: its exit code once it closed, or
    // nullopt when stopped. The window itself stays open when stopped.
    [[nodiscard]] std::optional<std::uint32_t> wait();
    // Ends a wait from another thread, for module unload. Stays set.
    void requestStop();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mgl::plugin
#endif
