// SPDX-License-Identifier: AGPL-3.0-only
#ifndef MGL_WINDOWS_APARTMENT_HPP
#define MGL_WINDOWS_APARTMENT_HPP

#include <winrt/base.h>

namespace mgl {
// Each diagnostic uses its own MTA; callers must not invoke it on an existing STA. Movable so a
// long-lived owner (such as a Wi-Fi group) can keep the apartment alive past the call that opened
// it; only the owning instance uninitialises.
class Apartment final {
  public:
    Apartment() { winrt::init_apartment(winrt::apartment_type::multi_threaded); }
    ~Apartment() {
        if (owns_) {
            winrt::uninit_apartment();
        }
    }
    Apartment(const Apartment&) = delete;
    Apartment& operator=(const Apartment&) = delete;
    Apartment(Apartment&& other) noexcept : owns_(other.owns_) { other.owns_ = false; }
    Apartment& operator=(Apartment&&) = delete;

  private:
    bool owns_ = true;
};
} // namespace mgl
#endif
