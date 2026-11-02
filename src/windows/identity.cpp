// SPDX-License-Identifier: AGPL-3.0-only
// Sealed storage on Windows: DPAPI (current-user scope) files under %LOCALAPPDATA%\MetaGLinkDirect.
#include "mgl/identity.hpp"
#include "identity_store.hpp"
#include "mgl/bytes.hpp"
#include "mgl/win_text.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <combaseapi.h>
#include <dpapi.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <knownfolders.h>
#include <minwindef.h>
#include <shlobj_core.h>
#include <winbase.h>
#include <winerror.h>
#include <winnt.h>

#include <cstddef>
#include <fstream>
#include <ios>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace mgl::identity {
namespace {
constexpr std::size_t MaxBlobSize = 1U << 20U; // 1 MiB ceiling on any sealed file

struct LocalFreeGuard {
    void operator()(void* pointer) const noexcept {
        if (pointer != nullptr) {
            LocalFree(pointer);
        }
    }
};

std::optional<std::string> localAppData() {
    PWSTR raw = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &raw) != S_OK) {
        CoTaskMemFree(raw);
        return std::nullopt;
    }
    std::string path = narrowUtf8(std::wstring_view(raw));
    CoTaskMemFree(raw);
    return path;
}

std::optional<std::string> filePath(const char* name) {
    const auto dir = storageDirectory();
    if (!dir) {
        return std::nullopt;
    }
    return *dir + "\\" + name;
}
} // namespace

std::optional<std::string> storageDirectory() {
    const auto base = localAppData();
    if (!base) {
        return std::nullopt;
    }
    auto dir = *base + "\\MetaGLinkDirect";
    if (CreateDirectoryA(dir.c_str(), nullptr) == FALSE && GetLastError() != ERROR_ALREADY_EXISTS) {
        return std::nullopt;
    }
    return dir;
}

namespace store {
bool save(const char* name, ByteView plaintext) {
    const auto path = filePath(name);
    if (!path) {
        return false;
    }
    // DPAPI's input DATA_BLOB is non-const; CryptProtectData only reads it.
    auto* const input =
        const_cast<BYTE*>(plaintext.data()); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    DATA_BLOB in{.cbData = static_cast<DWORD>(plaintext.size()), .pbData = input};
    DATA_BLOB out{};
    if (CryptProtectData(&in, L"MetaGLinkDirect", nullptr, nullptr, nullptr,
                         CRYPTPROTECT_UI_FORBIDDEN, &out) == FALSE) {
        noteProblem("Windows error " + std::to_string(GetLastError()));
        return false;
    }
    const std::unique_ptr<void, LocalFreeGuard> owned(out.pbData);
    // std::ofstream always adds out; out alone truncates exactly as out | trunc.
    std::ofstream file(*path, std::ios::binary);
    if (!file) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) — std::ofstream writes char.
    file.write(reinterpret_cast<const char*>(out.pbData), out.cbData);
    return file.good();
}

std::optional<Bytes> load(const char* name) {
    const auto path = filePath(name);
    if (!path) {
        return std::nullopt;
    }
    std::ifstream file(*path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    file.seekg(0, std::ios::end);
    const auto size = static_cast<std::streamoff>(file.tellg());
    if (size <= 0 || std::cmp_greater(size, MaxBlobSize)) {
        return std::nullopt;
    }
    Bytes sealed(static_cast<std::size_t>(size));
    file.seekg(0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) — std::ifstream reads char.
    file.read(reinterpret_cast<char*>(sealed.data()), size);
    if (!file) {
        return std::nullopt;
    }
    DATA_BLOB in{.cbData = static_cast<DWORD>(sealed.size()), .pbData = sealed.data()};
    DATA_BLOB out{};
    if (CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN,
                           &out) == FALSE) {
        noteProblem("Windows error " + std::to_string(GetLastError()));
        return std::nullopt;
    }
    const std::unique_ptr<void, LocalFreeGuard> owned(out.pbData);
    return Bytes(out.pbData, out.pbData + out.cbData);
}

bool remove(const char* name) {
    const auto path = filePath(name);
    if (!path) {
        return false;
    }
    return DeleteFileA(path->c_str()) != FALSE || GetLastError() == ERROR_FILE_NOT_FOUND;
}
} // namespace store
} // namespace mgl::identity
