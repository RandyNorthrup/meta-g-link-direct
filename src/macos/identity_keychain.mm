// SPDX-License-Identifier: AGPL-3.0-only
// Sealed storage on macOS: the user's login keychain. The setup tool writes the items and OBS's
// plugin reads them, so the item lists both as trusted when OBS is where macOS installs it; another
// reader gets the system's "allow access" question.
#include "identity_store.hpp"
#include "mgl/bytes.hpp"
#include "mgl/identity.hpp"
#include "mgl/platform.hpp"

#import <Foundation/Foundation.h>
#import <Security/Security.h>

#include <sys/stat.h>

#include <cerrno>
#include <cstdlib>
#include <optional>
#include <string>

namespace mgl::identity {
namespace {
NSString* const ServiceName = @"io.github.RandyNorthrup.MetaGLinkDirect";

NSDictionary* itemQuery(const char* name) {
    return @{
        (__bridge id)kSecClass : (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrService : ServiceName,
        (__bridge id)kSecAttrAccount : [NSString stringWithUTF8String:name],
    };
}

// The legacy access list is the only way a file-based keychain item can name other apps as trusted;
// the data-protection keychain would need an entitlement an unsigned beta cannot have. Tracked in
// PLAN.md.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
id trustedAccess() {
    NSMutableArray* trusted = [NSMutableArray array];
    SecTrustedApplicationRef self_app = nullptr;
    if (SecTrustedApplicationCreateFromPath(nullptr, &self_app) == errSecSuccess) {
        [trusted addObject:(__bridge_transfer id)self_app];
    }
    SecTrustedApplicationRef obs_app = nullptr;
    if (SecTrustedApplicationCreateFromPath("/Applications/OBS.app", &obs_app) == errSecSuccess) {
        [trusted addObject:(__bridge_transfer id)obs_app];
    }
    SecAccessRef access = nullptr;
    if (SecAccessCreate(CFSTR("Meta G Link Direct"), (__bridge CFArrayRef)trusted, &access) !=
        errSecSuccess) {
        return nil;
    }
    return (__bridge_transfer id)access;
}
#pragma clang diagnostic pop

void keepProblem(OSStatus status) {
    if (status != errSecSuccess && status != errSecItemNotFound) {
        store::noteProblem("keychain error " + std::to_string(status));
    }
}
} // namespace

std::optional<std::string> storageDirectory() {
    const char* home = std::getenv("HOME"); // NOLINT(concurrency-mt-unsafe)
    if (home == nullptr || *home != '/') {
        return std::nullopt;
    }
    std::string directory = std::string(home) + "/Library/Application Support/MetaGLinkDirect";
    if (mkdir(directory.c_str(), 0700) != 0 && errno != EEXIST) {
        return std::nullopt;
    }
    return directory;
}

namespace store {
bool save(const char* name, ByteView plaintext) {
    @autoreleasepool {
        (void)SecItemDelete((__bridge CFDictionaryRef)itemQuery(name));
        NSMutableDictionary* attributes = [itemQuery(name) mutableCopy];
        attributes[(__bridge id)kSecValueData] = [NSData dataWithBytes:plaintext.data()
                                                                length:plaintext.size()];
        attributes[(__bridge id)kSecAttrLabel] = @"Meta G Link Direct";
        if (id access = trustedAccess(); access != nil) {
            attributes[(__bridge id)kSecAttrAccess] = access;
        }
        const OSStatus status = SecItemAdd((__bridge CFDictionaryRef)attributes, nullptr);
        keepProblem(status);
        return status == errSecSuccess;
    }
}

std::optional<Bytes> load(const char* name) {
    @autoreleasepool {
        NSMutableDictionary* query = [itemQuery(name) mutableCopy];
        query[(__bridge id)kSecReturnData] = @YES;
        query[(__bridge id)kSecMatchLimit] = (__bridge id)kSecMatchLimitOne;
        CFTypeRef found = nullptr;
        const OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &found);
        keepProblem(status);
        if (status != errSecSuccess || found == nullptr) {
            return std::nullopt;
        }
        NSData* data = (__bridge_transfer NSData*)found;
        const auto* first = static_cast<const std::uint8_t*>(data.bytes);
        return Bytes(first, first + data.length);
    }
}

bool remove(const char* name) {
    @autoreleasepool {
        const OSStatus status = SecItemDelete((__bridge CFDictionaryRef)itemQuery(name));
        keepProblem(status);
        return status == errSecSuccess || status == errSecItemNotFound;
    }
}
} // namespace store
} // namespace mgl::identity
