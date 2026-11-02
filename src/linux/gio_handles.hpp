// SPDX-License-Identifier: AGPL-3.0-only
// Owning handles for the GLib objects the Linux D-Bus calls use, and the system bus connection.
#ifndef MGL_LINUX_GIO_HANDLES_HPP
#define MGL_LINUX_GIO_HANDLES_HPP

#include <gio/gio.h>

#include <memory>
#include <string>

namespace mgl::linux_bus {
struct VariantUnref {
    void operator()(GVariant* variant) const noexcept { g_variant_unref(variant); }
};
using VariantPtr = std::unique_ptr<GVariant, VariantUnref>;

struct ObjectUnref {
    void operator()(gpointer object) const noexcept { g_object_unref(object); }
};
using ConnectionPtr = std::unique_ptr<GDBusConnection, ObjectUnref>;

// Holds a GError an API may set; message() is empty when none was.
class Failure {
  public:
    Failure() = default;
    Failure(const Failure&) = delete;
    Failure& operator=(const Failure&) = delete;
    Failure(Failure&&) = delete;
    Failure& operator=(Failure&&) = delete;
    ~Failure() { g_clear_error(&error_); }
    [[nodiscard]] GError** out() noexcept { return &error_; }
    [[nodiscard]] std::string message() const {
        return error_ != nullptr ? std::string(error_->message) : std::string();
    }
    [[nodiscard]] int code() const noexcept { return error_ != nullptr ? error_->code : 0; }

  private:
    GError* error_ = nullptr;
};

// A private system bus connection for this caller; its name goes away when it is closed.
[[nodiscard]] inline ConnectionPtr systemBus(Failure& failure) {
    char* address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, nullptr, failure.out());
    if (address == nullptr) {
        return nullptr;
    }
    auto flags = static_cast<GDBusConnectionFlags>(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                                   G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION);
    ConnectionPtr connection(
        g_dbus_connection_new_for_address_sync(address, flags, nullptr, nullptr, failure.out()));
    g_free(address);
    return connection;
}
} // namespace mgl::linux_bus
#endif
