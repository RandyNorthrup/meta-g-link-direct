// SPDX-License-Identifier: AGPL-3.0-only
// WinHTTP implementation of mgl/http.hpp.
#include "mgl/http.hpp"

#include "mgl/bytes.hpp"
#include "mgl/win_text.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites for winhttp.h.

#include <minwindef.h>
#include <winhttp.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mgl::http {
namespace {
struct HandleCloser {
    void operator()(HINTERNET handle) const noexcept {
        if (handle != nullptr) {
            WinHttpCloseHandle(handle);
        }
    }
};
using Handle = std::unique_ptr<void, HandleCloser>;

} // namespace

Reply post(std::string_view url, const std::vector<std::pair<std::string, std::string>>& headers,
           ByteView body, std::string_view user_agent) {
    Reply reply;
    const auto wide_url = widenUtf8(url);
    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    std::array<wchar_t, 256> host{};
    std::array<wchar_t, 2048> path{};
    components.lpszHostName = host.data();
    components.dwHostNameLength = static_cast<DWORD>(host.size());
    components.lpszUrlPath = path.data();
    components.dwUrlPathLength = static_cast<DWORD>(path.size());
    if (WinHttpCrackUrl(wide_url.c_str(), 0, 0, &components) == FALSE) {
        reply.transport_error = L"bad url";
        return reply;
    }
    if (components.nScheme != INTERNET_SCHEME_HTTPS) {
        reply.transport_error = L"not https";
        return reply;
    }

    const Handle session(WinHttpOpen(widenUtf8(user_agent).c_str(),
                                     WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) {
        reply.transport_error = L"WinHttpOpen failed";
        return reply;
    }
    const Handle connect(
        WinHttpConnect(session.get(), std::wstring(host.data()).c_str(), components.nPort, 0));
    if (!connect) {
        reply.transport_error = L"WinHttpConnect failed";
        return reply;
    }
    const Handle request(
        WinHttpOpenRequest(connect.get(), L"POST", std::wstring(path.data()).c_str(), nullptr,
                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!request) {
        reply.transport_error = L"WinHttpOpenRequest failed";
        return reply;
    }

    std::wstring header_block;
    for (const auto& [name, value] : headers) {
        header_block += widenUtf8(name);
        header_block += L": ";
        header_block += widenUtf8(value);
        header_block += L"\r\n";
    }
    if (!header_block.empty()) {
        WinHttpAddRequestHeaders(request.get(), header_block.c_str(),
                                 static_cast<DWORD>(header_block.size()),
                                 WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    // CNG-style const-correctness: WinHTTP takes a non-const body pointer for an in buffer.
    auto* const body_ptr =
        const_cast<std::uint8_t*>(body.data()); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    if (WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, body_ptr,
                           static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()),
                           0) == FALSE) {
        reply.transport_error = L"WinHttpSendRequest failed";
        return reply;
    }
    if (WinHttpReceiveResponse(request.get(), nullptr) == FALSE) {
        reply.transport_error = L"WinHttpReceiveResponse failed";
        return reply;
    }

    DWORD status = 0;
    DWORD status_size = sizeof(status);
    WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size,
                        WINHTTP_NO_HEADER_INDEX);
    reply.status = status;

    for (;;) {
        DWORD available = 0;
        if (WinHttpQueryDataAvailable(request.get(), &available) == FALSE) {
            reply.transport_error = L"WinHttpQueryDataAvailable failed";
            return reply;
        }
        if (available == 0) {
            break;
        }
        const auto offset = reply.body.size();
        reply.body.resize(offset + available);
        DWORD read = 0;
        if (WinHttpReadData(request.get(), reply.body.data() + offset, available, &read) == FALSE) {
            reply.transport_error = L"WinHttpReadData failed";
            return reply;
        }
        reply.body.resize(offset + read);
        if (read == 0) {
            break;
        }
    }
    return reply;
}
} // namespace mgl::http
