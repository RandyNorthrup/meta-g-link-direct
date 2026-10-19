// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/identity.hpp"

#include "mgl/bytes.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <dpapi.h>
#include <shlobj.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <optional>
#include <string>

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
    const int needed = WideCharToMultiByte(CP_UTF8, 0, raw, -1, nullptr, 0, nullptr, nullptr);
    std::string path(static_cast<std::size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, raw, -1, path.data(), needed, nullptr, nullptr);
    CoTaskMemFree(raw);
    if (!path.empty() && path.back() == '\0') {
        path.pop_back();
    }
    return path;
}

std::optional<std::string> filePath(const char* name) {
    const auto dir = storageDirectory();
    if (!dir) {
        return std::nullopt;
    }
    return *dir + "\\" + name;
}

bool sealToFile(const std::string& path, ByteView plaintext) {
    DATA_BLOB in{.cbData = static_cast<DWORD>(plaintext.size()),
                 .pbData = const_cast<BYTE*>(
                     plaintext.data())}; // NOLINT(cppcoreguidelines-pro-type-const-cast)
    DATA_BLOB out{};
    if (CryptProtectData(&in, L"MetaGLinkDirect", nullptr, nullptr, nullptr,
                         CRYPTPROTECT_UI_FORBIDDEN, &out) == FALSE) {
        return false;
    }
    const std::unique_ptr<void, LocalFreeGuard> owned(out.pbData);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(out.pbData),
               out.cbData); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return file.good();
}

std::optional<Bytes> unsealFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return std::nullopt;
    }
    const auto size = static_cast<std::streamoff>(file.tellg());
    if (size <= 0 || static_cast<std::size_t>(size) > MaxBlobSize) {
        return std::nullopt;
    }
    Bytes sealed(static_cast<std::size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(sealed.data()),
              size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    if (!file) {
        return std::nullopt;
    }
    DATA_BLOB in{.cbData = static_cast<DWORD>(sealed.size()), .pbData = sealed.data()};
    DATA_BLOB out{};
    if (CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN,
                           &out) == FALSE) {
        return std::nullopt;
    }
    const std::unique_ptr<void, LocalFreeGuard> owned(out.pbData);
    return Bytes(out.pbData, out.pbData + out.cbData);
}

void appendString(Bytes& out, const std::string& text) {
    const auto size = static_cast<std::uint32_t>(text.size());
    for (unsigned shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<std::uint8_t>((size >> shift) & 0xFF));
    }
    out.insert(out.end(), text.begin(), text.end());
}

bool readString(ByteView data, std::size_t& offset, std::string& out) {
    if (data.size() - offset < sizeof(std::uint32_t)) {
        return false;
    }
    std::uint32_t size = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        size |= static_cast<std::uint32_t>(data[offset++]) << shift;
    }
    if (data.size() - offset < size) {
        return false;
    }
    out.assign(reinterpret_cast<const char*>(data.data() + offset),
               size); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    offset += size;
    return true;
}
} // namespace

std::optional<std::string> storageDirectory() {
    const auto base = localAppData();
    if (!base) {
        return std::nullopt;
    }
    const auto dir = *base + "\\MetaGLinkDirect";
    if (CreateDirectoryA(dir.c_str(), nullptr) == FALSE && GetLastError() != ERROR_ALREADY_EXISTS) {
        return std::nullopt;
    }
    return dir;
}

bool saveOwnerIdentity(const OwnerIdentity& identity) {
    const auto path = filePath("identity.bin");
    if (!path) {
        return false;
    }
    Bytes blob;
    const auto key_size = static_cast<std::uint32_t>(identity.owner_private.size());
    for (unsigned shift = 0; shift < 32; shift += 8) {
        blob.push_back(static_cast<std::uint8_t>((key_size >> shift) & 0xFF));
    }
    append(blob, identity.owner_private);
    appendString(blob, identity.serial);
    appendString(blob, identity.user_id);
    const bool ok = sealToFile(*path, blob);
    SecureZeroMemory(blob.data(), blob.size());
    return ok;
}

std::optional<OwnerIdentity> loadOwnerIdentity() {
    const auto path = filePath("identity.bin");
    if (!path) {
        return std::nullopt;
    }
    auto blob = unsealFromFile(*path);
    if (!blob) {
        return std::nullopt;
    }
    std::size_t offset = 0;
    const ByteView view(*blob);
    if (view.size() < sizeof(std::uint32_t)) {
        return std::nullopt;
    }
    std::uint32_t key_size = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        key_size |= static_cast<std::uint32_t>(view[offset++]) << shift;
    }
    if (view.size() - offset < key_size) {
        return std::nullopt;
    }
    OwnerIdentity identity;
    identity.owner_private.assign(view.begin() + static_cast<std::ptrdiff_t>(offset),
                                  view.begin() + static_cast<std::ptrdiff_t>(offset + key_size));
    offset += key_size;
    if (!readString(view, offset, identity.serial) || !readString(view, offset, identity.user_id)) {
        return std::nullopt;
    }
    return identity;
}

bool forgetOwnerIdentity() {
    const auto path = filePath("identity.bin");
    if (!path) {
        return false;
    }
    return DeleteFileA(path->c_str()) != FALSE || GetLastError() == ERROR_FILE_NOT_FOUND;
}

bool saveTokenBlob(const std::string& text) {
    const auto path = filePath("tokens.bin");
    return path &&
           sealToFile(
               *path,
               ByteView(reinterpret_cast<const std::uint8_t*>(
                            text.data()), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                        text.size()));
}

std::optional<std::string> loadTokenBlob() {
    const auto path = filePath("tokens.bin");
    if (!path) {
        return std::nullopt;
    }
    auto blob = unsealFromFile(*path);
    if (!blob) {
        return std::nullopt;
    }
    return std::string(reinterpret_cast<const char*>(blob->data()),
                       blob->size()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}
} // namespace mgl::identity
