// SPDX-License-Identifier: AGPL-3.0-only
#ifndef MGL_WINDOWS_APARTMENT_HPP
#define MGL_WINDOWS_APARTMENT_HPP

#include <winrt/base.h>

namespace mgl {
// Each diagnostic uses its own MTA; callers must not invoke it on an existing STA.
class Apartment final {
  public:
    Apartment() { winrt::init_apartment(winrt::apartment_type::multi_threaded); }
    ~Apartment() { winrt::uninit_apartment(); }
    Apartment(const Apartment&) = delete;
    Apartment& operator=(const Apartment&) = delete;
    Apartment(Apartment&&) = delete;
    Apartment& operator=(Apartment&&) = delete;
};
} // namespace mgl
#endif
