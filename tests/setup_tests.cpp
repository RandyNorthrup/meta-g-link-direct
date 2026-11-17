// SPDX-License-Identifier: AGPL-3.0-only
// The check every setup window makes on the address a login ended on, and how setup reads Meta's
// answer to the login code.
#include "mgl/provision.hpp"
#include "mgl/setup.hpp"
#include "mgl/wifi_station.hpp"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void check(bool ok, const char* name) {
    if (!ok) {
        throw std::runtime_error(std::string(name) + ": failed");
    }
}

constexpr const char* Redirect = "https://accounts.meta.com/native/oidc/redirect/metaai";

void testLoginAddress() {
    const std::string base(Redirect);
    const auto good = mgl::setup::readLoginAddress(base + "?code=abc&state=ours", "ours");
    check(good.code == "abc" && good.problem.empty(), "the login's own address gives its code");

    const auto other = mgl::setup::readLoginAddress(base + "?code=abc&state=theirs", "ours");
    check(!other.code.has_value(), "another login's address gives no code");
    check(other.problem.find("different login") != std::string::npos,
          "another login's address says so");

    const auto empty = mgl::setup::readLoginAddress(base + "?state=ours", "ours");
    check(!empty.code.has_value(), "an address without a code gives none");
    check(empty.problem.find("did not return a code") != std::string::npos,
          "an address without a code says so");

    const auto pasted = mgl::setup::readLoginAddress(base + "?code=abc", "ours");
    check(pasted.code == "abc", "an address without a state still gives its code");
}

// Meta's answers to a token exchange, as observed on 2026-11-17: a fresh code gives JSON with
// access_token and user_id; the same code sent again gives HTTP 500 with a plain-text body.
void testTokenExchangeReply() {
    using mgl::provision::readTokenExchange;
    constexpr unsigned Ok = 200;
    constexpr unsigned BadRequest = 400;
    constexpr unsigned ServerError = 500;
    const auto fresh = readTokenExchange(Ok, R"({"access_token":"t","user_id":1})");
    check(fresh.frla == "t" && fresh.error.empty(), "a fresh code's reply gives its token");

    const auto reused = readTokenExchange(ServerError, "Internal Server Error");
    check(!reused.frla.has_value(), "a refused code gives no token");
    check(reused.error == mgl::provision::RefusedLoginCode, "a refused code says it was refused");

    const auto json_error =
        readTokenExchange(BadRequest, R"({"error":{"message":"Invalid code"}})");
    check(json_error.error == "Invalid code", "a JSON error keeps Meta's own message");

    const auto server_json = readTokenExchange(ServerError, R"({"error":{"message":"Try later"}})");
    check(server_json.error == "Try later", "a JSON 500 keeps Meta's own message");

    const auto empty = readTokenExchange(Ok, R"({"user_id":1})");
    check(!empty.frla.has_value() && empty.error.find("no access_token") != std::string::npos,
          "a reply without a token says so");

    const auto offline = readTokenExchange(0, "");
    check(offline.error.find("network error") != std::string::npos, "no answer says so");
}

void testAdapterPresence() {
    check(!mgl::wifiAdapterPresent("no-such-adapter"), "an adapter nobody has is not present");
    for (const auto& adapter : mgl::listWifiAdapters()) {
        check(mgl::wifiAdapterPresent(adapter.id), "a listed adapter is present");
    }
}
} // namespace

int main() {
    try {
        testLoginAddress();
        testTokenExchangeReply();
        testAdapterPresence();
        std::cout << "setup cases passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        constexpr int DiagnosticWriteFailed = 4;
        if (std::fputs(error.what(), stderr) == EOF || std::fputc('\n', stderr) == EOF) {
            return DiagnosticWriteFailed;
        }
        return EXIT_FAILURE;
    }
}
