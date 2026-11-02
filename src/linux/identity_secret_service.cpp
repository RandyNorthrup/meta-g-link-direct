// SPDX-License-Identifier: AGPL-3.0-only
// Sealed storage on Linux: the desktop's Secret Service (GNOME Keyring, KWallet) through libsecret.
// There is no fallback to a plain file: without a Secret Service, setup says so and stops. Inside a
// Flatpak, where the desktop offers the Secret portal, libsecret instead keeps the blobs in the
// app's own file, encrypted with a key from that portal, so the OBS Flatpak has its own identity.
#include "identity_store.hpp"
#include "mgl/bytes.hpp"
#include "mgl/encoding.hpp"
#include "mgl/identity.hpp"
#include "mgl/platform.hpp"

#include <glib.h>
#include <libsecret/secret.h>
#include <sys/stat.h>

#include <cerrno>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

namespace mgl::identity {
namespace {
const SecretSchema& schema() {
    static const SecretSchema instance = [] {
        SecretSchema value{};
        value.name = "io.github.RandyNorthrup.MetaGLinkDirect";
        value.flags = SECRET_SCHEMA_NONE;
        value.attributes[0] = {"blob", SECRET_SCHEMA_ATTRIBUTE_STRING};
        value.attributes[1] = {nullptr, static_cast<SecretSchemaAttributeType>(0)};
        return value;
    }();
    return instance;
}

// mkdir -p for the one path this project uses.
bool makeDirectories(const std::string& path) {
    for (std::size_t slash = path.find('/', 1); slash != std::string::npos;
         slash = path.find('/', slash + 1)) {
        if (mkdir(path.substr(0, slash).c_str(), 0700) != 0 && errno != EEXIST) {
            return false;
        }
    }
    return mkdir(path.c_str(), 0700) == 0 || errno == EEXIST;
}

void keepProblem(GError*& error) {
    if (error != nullptr) {
        store::noteProblem(error->message);
    }
    g_clear_error(&error);
}
} // namespace

std::optional<std::string> storageDirectory() {
    // XDG_DATA_HOME or ~/.local/share; inside a Flatpak this is the app's own data folder.
    const char* data_home = std::getenv("XDG_DATA_HOME"); // NOLINT(concurrency-mt-unsafe)
    std::string base;
    if (data_home != nullptr && *data_home == '/') {
        base = data_home;
    } else {
        const char* home = std::getenv("HOME"); // NOLINT(concurrency-mt-unsafe)
        if (home == nullptr || *home != '/') {
            return std::nullopt;
        }
        base = std::string(home) + "/.local/share";
    }
    auto directory = base + "/MetaGLinkDirect";
    if (!makeDirectories(directory)) {
        return std::nullopt;
    }
    return directory;
}

namespace store {
bool save(const char* name, ByteView plaintext) {
    // The Secret Service stores text, so the blob goes as base64.
    std::string text = encoding::base64Encode(plaintext);
    GError* error = nullptr;
    const std::string label = std::string("Meta G Link Direct ") + name;
    const gboolean stored =
        secret_password_store_sync(&schema(), SECRET_COLLECTION_DEFAULT, label.c_str(),
                                   text.c_str(), nullptr, &error, "blob", name, nullptr);
    platform::secureZero(text.data(), text.size());
    keepProblem(error);
    return stored != FALSE;
}

std::optional<Bytes> load(const char* name) {
    GError* error = nullptr;
    gchar* const secret =
        secret_password_lookup_sync(&schema(), nullptr, &error, "blob", name, nullptr);
    keepProblem(error);
    if (secret == nullptr) {
        return std::nullopt;
    }
    Bytes blob = encoding::base64Decode(secret);
    secret_password_free(secret);
    return blob;
}

bool remove(const char* name) {
    GError* error = nullptr;
    (void)secret_password_clear_sync(&schema(), nullptr, &error, "blob", name, nullptr);
    const bool ok = error == nullptr;
    keepProblem(error);
    return ok;
}
} // namespace store
} // namespace mgl::identity
