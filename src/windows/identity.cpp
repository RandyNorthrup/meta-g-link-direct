// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/identity.hpp"

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
#include <cstdint>
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

bool sealToFile(const std::string& path, ByteView plaintext) {
    // DPAPI's input DATA_BLOB is non-const; CryptProtectData only reads it.
    auto* const input =
        const_cast<BYTE*>(plaintext.data()); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    DATA_BLOB in{.cbData = static_cast<DWORD>(plaintext.size()), .pbData = input};
    DATA_BLOB out{};
    if (CryptProtectData(&in, L"MetaGLinkDirect", nullptr, nullptr, nullptr,
                         CRYPTPROTECT_UI_FORBIDDEN, &out) == FALSE) {
        return false;
    }
    const std::unique_ptr<void, LocalFreeGuard> owned(out.pbData);
    // std::ofstream always adds out; out alone truncates exactly as out | trunc.
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) — std::ofstream writes char.
    file.write(reinterpret_cast<const char*>(out.pbData), out.cbData);
    return file.good();
}

std::optional<Bytes> unsealFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
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
        return std::nullopt;
    }
    const std::unique_ptr<void, LocalFreeGuard> owned(out.pbData);
    return Bytes(out.pbData, out.pbData + out.cbData);
}

// Reads a little-endian u32 at offset and advances past it; nullopt if fewer than four bytes
// remain.
std::optional<std::uint32_t> readU32(ByteView data, std::size_t& offset) {
    if (data.size() - offset < sizeof(std::uint32_t)) {
        return std::nullopt;
    }
    std::uint32_t value = 0;
    unsigned shift = 0;
    for (const std::uint8_t byte : data.subspan(offset, sizeof(std::uint32_t))) {
        value |= static_cast<std::uint32_t>(byte) << shift;
        shift += 8U;
    }
    offset += sizeof(std::uint32_t);
    return value;
}

void appendString(Bytes& out, const std::string& text) {
    appendLittleEndian(out, static_cast<std::uint32_t>(text.size()));
    out.insert(out.end(), text.begin(), text.end());
}

bool readString(ByteView data, std::size_t& offset, std::string& out) {
    const auto size = readU32(data, offset);
    if (!size || data.size() - offset < *size) {
        return false;
    }
    const auto text = data.subspan(offset, *size);
    out.assign(text.begin(), text.end());
    offset += *size;
    return true;
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

bool saveOwnerIdentity(const OwnerIdentity& identity) {
    const auto path = filePath("identity.bin");
    if (!path) {
        return false;
    }
    Bytes blob;
    appendLittleEndian(blob, static_cast<std::uint32_t>(identity.owner_private.size()));
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
    const auto key_size = readU32(view, offset);
    if (!key_size || view.size() - offset < *key_size) {
        return std::nullopt;
    }
    OwnerIdentity identity;
    const auto key = view.subspan(offset, *key_size);
    identity.owner_private.assign(key.begin(), key.end());
    offset += *key_size;
    if (!readString(view, offset, identity.serial) || !readString(view, offset, identity.user_id)) {
        return std::nullopt;
    }
    return identity;
}

namespace {
bool deleteSealedFile(const char* name) {
    const auto path = filePath(name);
    if (!path) {
        return false;
    }
    return DeleteFileA(path->c_str()) != FALSE || GetLastError() == ERROR_FILE_NOT_FOUND;
}
} // namespace

bool forgetOwnerIdentity() {
    // Both run even if the first fails, so a partial Forget removes all it can.
    const bool identity_gone = deleteSealedFile("identity.bin");
    const bool tokens_gone = deleteSealedFile("tokens.bin");
    return identity_gone && tokens_gone;
}

bool saveTokenBlob(const std::string& text) {
    const auto path = filePath("tokens.bin");
    if (!path) {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) — byte view of the token text.
    const ByteView bytes(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
    return sealToFile(*path, bytes);
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
    return std::string(blob->begin(), blob->end());
}
} // namespace mgl::identity
