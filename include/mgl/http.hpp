// SPDX-License-Identifier: AGPL-3.0-only
// The one HTTP shape the Meta setup calls use: an HTTPS POST with headers and a body, returning
// the status and the response bytes. WinHTTP implementation; failures surface as status 0.
#ifndef MGL_HTTP_HPP
#define MGL_HTTP_HPP

#include "mgl/bytes.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mgl::http {
struct Reply {
    unsigned status = 0; // 0 means the request did not complete
    Bytes body;
    std::wstring transport_error; // populated when status is 0
};

// POST body to an https:// url with the given headers (name,value). The caller sets Content-Type
// and any Content-Encoding. The user agent is sent as given.
[[nodiscard]] Reply post(std::string_view url,
                         const std::vector<std::pair<std::string, std::string>>& headers,
                         ByteView body, std::string_view user_agent);
} // namespace mgl::http
#endif
