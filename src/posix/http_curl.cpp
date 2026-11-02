// SPDX-License-Identifier: AGPL-3.0-only
// libcurl implementation of mgl/http.hpp, for the Meta setup calls.
#include "mgl/bytes.hpp"
#include "mgl/http.hpp"

#include <curl/curl.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mgl::http {
namespace {
constexpr long RequestTimeoutSeconds = 60;
constexpr long ConnectTimeoutSeconds = 20;

struct EasyCleanup {
    void operator()(CURL* easy) const noexcept { curl_easy_cleanup(easy); }
};
struct ListFree {
    void operator()(curl_slist* list) const noexcept { curl_slist_free_all(list); }
};

std::size_t collect(char* data, std::size_t size, std::size_t count, void* target) {
    auto* const body = static_cast<Bytes*>(target);
    const std::size_t total = size * count;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) — libcurl hands bytes as char.
    const auto* const first = reinterpret_cast<const std::uint8_t*>(data);
    body->insert(body->end(), first, first + total);
    return total;
}
} // namespace

Reply post(std::string_view url, const std::vector<std::pair<std::string, std::string>>& headers,
           ByteView body, std::string_view user_agent) {
    Reply reply;
    if (!url.starts_with("https://")) {
        reply.transport_error = "not https";
        return reply;
    }
    static std::once_flag global;
    std::call_once(global, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });

    const std::unique_ptr<CURL, EasyCleanup> easy(curl_easy_init());
    if (!easy) {
        reply.transport_error = "curl_easy_init failed";
        return reply;
    }
    std::unique_ptr<curl_slist, ListFree> header_list;
    for (const auto& [name, value] : headers) {
        const std::string line = name + ": " + value;
        curl_slist* const extended = curl_slist_append(header_list.get(), line.c_str());
        if (extended == nullptr) {
            reply.transport_error = "curl_slist_append failed";
            return reply;
        }
        (void)header_list.release();
        header_list.reset(extended);
    }
    const std::string url_text(url);
    const std::string agent_text(user_agent);
    curl_easy_setopt(easy.get(), CURLOPT_URL, url_text.c_str());
    curl_easy_setopt(easy.get(), CURLOPT_USERAGENT, agent_text.c_str());
    curl_easy_setopt(easy.get(), CURLOPT_HTTPHEADER, header_list.get());
    curl_easy_setopt(easy.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(easy.get(), CURLOPT_POSTFIELDS, body.data());
    curl_easy_setopt(easy.get(), CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size()));
    curl_easy_setopt(easy.get(), CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(easy.get(), CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(easy.get(), CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(easy.get(), CURLOPT_TIMEOUT, RequestTimeoutSeconds);
    curl_easy_setopt(easy.get(), CURLOPT_CONNECTTIMEOUT, ConnectTimeoutSeconds);
    curl_easy_setopt(easy.get(), CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easy.get(), CURLOPT_WRITEFUNCTION, &collect);
    curl_easy_setopt(easy.get(), CURLOPT_WRITEDATA, &reply.body);

    const CURLcode performed = curl_easy_perform(easy.get());
    if (performed != CURLE_OK) {
        reply.transport_error = curl_easy_strerror(performed);
        reply.status = 0;
        return reply;
    }
    long status = 0;
    curl_easy_getinfo(easy.get(), CURLINFO_RESPONSE_CODE, &status);
    reply.status = static_cast<unsigned>(status);
    return reply;
}
} // namespace mgl::http
