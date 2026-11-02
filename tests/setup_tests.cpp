// SPDX-License-Identifier: AGPL-3.0-only
// The check every setup window makes on the address a login ended on.
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
