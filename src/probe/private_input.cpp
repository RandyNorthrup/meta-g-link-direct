// SPDX-License-Identifier: AGPL-3.0-only
#include "private_input.hpp"
#include "mgl/address.hpp"

#include <windows.h> // IWYU pragma: keep — Windows SDK prerequisites.

#include <consoleapi.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <minwindef.h>
#include <processenv.h>
#include <winbase.h>
#include <winerror.h>
#include <winnt.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace mgl {
namespace {
class ConsoleEcho final {
  public:
    explicit ConsoleEcho(PrivateAddressInput& result)
        : result_(result), input_(GetStdHandle(STD_INPUT_HANDLE)) {
        if (GetConsoleMode(input_, &original_) != FALSE) {
            console_ = true;
            if (SetConsoleMode(input_, original_ & ~static_cast<DWORD>(ENABLE_ECHO_INPUT)) ==
                FALSE) {
                result_.error = GetLastError();
            } else {
                changed_ = true;
            }
        } else {
            const auto type = GetFileType(input_);
            if (type != FILE_TYPE_PIPE && type != FILE_TYPE_DISK) {
                result_.error = ERROR_INVALID_HANDLE;
            }
        }
    }
    ~ConsoleEcho() {
        if (changed_ && SetConsoleMode(input_, original_) == FALSE) {
            result_.error = GetLastError();
        }
    }
    ConsoleEcho(const ConsoleEcho&) = delete;
    ConsoleEcho& operator=(const ConsoleEcho&) = delete;
    ConsoleEcho(ConsoleEcho&&) = delete;
    ConsoleEcho& operator=(ConsoleEcho&&) = delete;
    [[nodiscard]] bool console() const noexcept { return console_; }

  private:
    PrivateAddressInput& result_;
    HANDLE input_;
    DWORD original_ = 0;
    bool console_ = false;
    bool changed_ = false;
};
} // namespace

PrivateAddressInput readPrivateAddress() {
    PrivateAddressInput result{};
    {
        const ConsoleEcho echo(result);
        if (result.error == 0) {
            if (echo.console()) {
                std::cerr << "Enter glasses Bluetooth address locally (input hidden): "
                          << std::flush;
            }
            constexpr std::size_t MaximumInputBuffer = 18;
            std::array<char, MaximumInputBuffer> input{};
            if (std::cin.getline(input.data(), static_cast<std::streamsize>(input.size()))) {
                const auto length = std::cin.gcount() - (std::cin.eof() ? 0 : 1);
                result.value = parseBluetoothAddress(
                    std::string_view(input.data(), static_cast<std::size_t>(length)));
            }
            if (echo.console()) {
                std::cerr << '\n';
            }
        }
    }
    return result;
}

bool writeRequestedReport(std::string_view redacted_json) {
    constexpr DWORD MaximumPathBuffer = 32768;
    std::array<wchar_t, MaximumPathBuffer> path{};
    SetLastError(ERROR_SUCCESS);
    const auto length =
        GetEnvironmentVariableW(L"MGL_PROBE_REPORT", path.data(), MaximumPathBuffer);
    if (length == 0) {
        return GetLastError() == ERROR_ENVVAR_NOT_FOUND || GetLastError() == ERROR_SUCCESS;
    }
    if (length >= path.size()) {
        return false;
    }
    std::ofstream output(std::filesystem::path(std::wstring_view(path.data(), length)),
                         std::ios::binary);
    output << redacted_json;
    output.close();
    return static_cast<bool>(output);
}
} // namespace mgl
