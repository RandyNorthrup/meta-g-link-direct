// SPDX-License-Identifier: AGPL-3.0-only
// Small encoders the Meta HTTP calls need: standard base64, percent-encoding matching Python's
// urllib.parse.quote_plus, and a gzip wrapper using stored (uncompressed) DEFLATE blocks so the
// Content-Encoding: gzip body the Meta app sends is reproduced without a compression library.
#ifndef MGL_ENCODING_HPP
#define MGL_ENCODING_HPP

#include "mgl/bytes.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mgl::encoding {
[[nodiscard]] std::string base64Encode(ByteView data);
[[nodiscard]] Bytes base64Decode(std::string_view text); // skips non-alphabet chars, like Python

// application/x-www-form-urlencoded in insertion order, quote_plus for keys and values.
[[nodiscard]] std::string
formUrlEncode(const std::vector<std::pair<std::string, std::string>>& fields);
[[nodiscard]] std::string quotePlus(std::string_view value);

// The percent-decoded value of query parameter name in an address as a browser's address bar
// shows it after a redirect: "https://host/path?state=x&code=AbC%2D1#_=_" gives "AbC-1" for
// "code". Surrounding whitespace is ignored. nullopt when the parameter is absent or empty, or its
// percent-encoding is malformed.
[[nodiscard]] std::optional<std::string> queryParameter(std::string_view address,
                                                        std::string_view name);

// A valid gzip stream wrapping data in stored DEFLATE blocks (no size growth concern here).
[[nodiscard]] Bytes gzip(ByteView data);
} // namespace mgl::encoding
#endif
