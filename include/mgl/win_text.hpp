// SPDX-License-Identifier: AGPL-3.0-only
// UTF-8 to and from the UTF-16 that Windows wide-character APIs take. Invalid input converts as
// Windows converts it (replacement characters); an empty input gives an empty result.
#ifndef MGL_WIN_TEXT_HPP
#define MGL_WIN_TEXT_HPP

#include <string>
#include <string_view>

namespace mgl {
[[nodiscard]] std::wstring widenUtf8(std::string_view utf8);
[[nodiscard]] std::string narrowUtf8(std::wstring_view wide);
} // namespace mgl
#endif
