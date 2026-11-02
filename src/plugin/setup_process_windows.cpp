// SPDX-License-Identifier: AGPL-3.0-only
#include "setup_process.hpp"

#include "mgl/identity.hpp"
#include "mgl/win_text.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <processthreadsapi.h>
#include <synchapi.h>
#include <winbase.h>
#include <winnt.h>

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace mgl::plugin {
namespace {
struct HandleCloser {
    void operator()(HANDLE handle) const noexcept {
        if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
    }
};
using UniqueHandle = std::unique_ptr<void, HandleCloser>;

// The installed setup window, %LOCALAPPDATA%\MetaGLinkDirect\bin\meta-g-link-setup.exe, when
// present.
std::optional<std::wstring> toolPath() {
    const auto directory = mgl::identity::storageDirectory();
    if (!directory) {
        return std::nullopt;
    }
    std::wstring path = mgl::widenUtf8(*directory) + L"\\bin\\meta-g-link-setup.exe";
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return std::nullopt;
    }
    return path;
}
} // namespace

struct SetupProcess::Impl {
    UniqueHandle process;
    UniqueHandle stop_event{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
};

SetupProcess::SetupProcess() : impl_(std::make_unique<Impl>()) {}
SetupProcess::~SetupProcess() = default;

SetupProcess::Start SetupProcess::start(std::uint32_t& error) {
    const auto tool = toolPath();
    if (!tool) {
        return Start::ToolMissing;
    }
    if (!impl_->stop_event) {
        error = GetLastError();
        return Start::Failed;
    }
    std::wstring command = L"\"" + *tool + L"\"";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (CreateProcessW(tool->c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr,
                       &startup, &process) == FALSE) {
        error = GetLastError();
        return Start::Failed;
    }
    CloseHandle(process.hThread);
    impl_->process.reset(process.hProcess);
    return Start::Started;
}

std::optional<std::uint32_t> SetupProcess::wait() {
    const std::array<HANDLE, 2> waits{impl_->process.get(), impl_->stop_event.get()};
    const DWORD woke =
        WaitForMultipleObjects(static_cast<DWORD>(waits.size()), waits.data(), FALSE, INFINITE);
    if (woke != WAIT_OBJECT_0) {
        return std::nullopt; // unload, or the wait failed; the window stays open on its own
    }
    DWORD code = 0;
    GetExitCodeProcess(impl_->process.get(), &code);
    return code;
}

void SetupProcess::requestStop() {
    if (impl_->stop_event) {
        SetEvent(impl_->stop_event.get());
    }
}
} // namespace mgl::plugin
