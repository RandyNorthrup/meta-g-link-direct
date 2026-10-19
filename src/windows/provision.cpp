// SPDX-License-Identifier: AGPL-3.0-only
// Port of Visor core/identity token/WebLogin.kt, token/Frla.kt and provision/Provision.kt at
// e7077b4051b1e3e605c667aa2a010fbe41f346a0. See THIRD_PARTY_NOTICES.md.
#include "mgl/provision.hpp"

#include "mgl/bytes.hpp"
#include "mgl/crypto.hpp"
#include "mgl/datax.hpp"
#include "mgl/encoding.hpp"
#include "mgl/http.hpp"
#include "mgl/json.hpp"
#include "mgl/protobuf.hpp"
#include "mgl/session.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mgl::provision {
namespace {
using namespace std::chrono_literals;

constexpr std::string_view AppId = "934401142814369";
constexpr std::string_view RedirectUri = "https://accounts.meta.com/native/oidc/redirect/metaai";
constexpr std::string_view AuthorizeUrl = "https://accounts.meta.com/oidc/";
constexpr std::string_view ExchangeUrl =
    "https://api.accounts.meta.com/native/oidc/token-exchange/";
constexpr std::string_view ArLoginUrl = "https://ar.graph.meta.com/login";
constexpr std::string_view GraphBase = "https://graph.facebook-hardware.com";
constexpr std::string_view UserAgent =
    "Dalvik/2.1.0 (Linux; U; Android 15) "
    "[FBAN/Stella;FBAV/289.0.0.25.162;FBSN/Android;FBSV/15;FBLC/en_US]";
constexpr std::string_view FormUrlEncoded = "application/x-www-form-urlencoded";

constexpr std::uint16_t IdentityService = 36;
constexpr std::uint16_t MtIdentityRequest = 0x3000;
constexpr std::uint16_t MtIdentityResponse = 0x3001;
constexpr std::uint16_t MtSkipChallenge = 0x2000;
constexpr std::uint16_t MtSkipChallengeAck = 0x2001;
constexpr std::uint16_t MtStartChangeOwnerReq = 0x2002;
constexpr std::uint16_t MtStartChangeOwnerResp = 0x2003;
constexpr std::uint16_t MtFinishChangeOwnerReq = 0x2004;
constexpr std::uint16_t MtFinishChangeOwnerResp = 0x2005;
constexpr auto StepTimeout = 20s;

ByteView view(std::string_view text) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) — byte view of the form text.
    return {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
}

bool httpDebug() {
    char* raw = nullptr;
    std::size_t size = 0;
    // NOLINTNEXTLINE(misc-include-cleaner) - UCRT-only; declared in <stdlib.h>, under <cstdlib>.
    const auto status = _dupenv_s(&raw, &size, "MGL_HTTP_DEBUG");
    // _dupenv_s hands back a CRT allocation the caller must release with free.
    const std::unique_ptr<char, decltype(&std::free)> value(raw, &std::free);
    return status == 0 && value != nullptr;
}

std::string toString(ByteView data) { return {data.begin(), data.end()}; }

// Debug traces name request fields and reply keys but never print values: request bodies carry
// the app and user tokens, and replies carry ownership receipts and account ids. Trace writes are
// best effort; a failed stderr write does not affect the request.
void debugRequest(std::string_view url,
                  const std::vector<std::pair<std::string, std::string>>& fields) {
    std::string line = "[post] " + std::string(url) + " fields:";
    for (const auto& field : fields) {
        line += ' ';
        line += field.first;
    }
    line += '\n';
    (void)std::fputs(line.c_str(), stderr);
}

void debugReply(std::string_view label, unsigned status, ByteView body) {
    std::string line = std::string(label) + " HTTP " + std::to_string(status);
    const auto parsed = json::parse(toString(body));
    if (parsed && parsed->kind() == json::Kind::Object) {
        line += " keys:";
        for (const auto& member : parsed->object()) {
            line += ' ';
            line += member.first;
        }
        if (const auto* err = parsed->at("error")) {
            line += " error: " + err->stringAt("message").value_or("(no message)");
        }
    }
    line += '\n';
    (void)std::fputs(line.c_str(), stderr);
}

std::string randomState() {
    std::array<std::uint8_t, 16> raw{};
    crypto::randomBytes(raw);
    // base64url without padding, as secrets.token_urlsafe.
    auto text = encoding::base64Encode(ByteView(raw.data(), raw.size()));
    std::string out;
    for (const char ch : text) {
        if (ch == '+') {
            out.push_back('-');
        } else if (ch == '/') {
            out.push_back('_');
        } else if (ch != '=') {
            out.push_back(ch);
        }
    }
    return out;
}

// An ownership receipt as carried over svc36: {1: signature, 2: JSON body}.
struct Receipt {
    Bytes signature;
    Bytes body_json;
    [[nodiscard]] Bytes encode() const {
        Bytes out;
        proto::appendBytesField(out, 1, signature);
        proto::appendBytesField(out, 2, body_json);
        return out;
    }
};

// Reads "<key>_ownership_receipt" (string or object) and "receipt_signature" (base64) from a
// Meta hardware reply.
std::optional<Receipt> parseReceipt(const json::Value& reply, std::string_view receipt_key,
                                    std::string& error) {
    if (const auto* err = reply.at("error")) {
        error = err->stringAt("message").value_or("Meta returned an error");
        return std::nullopt;
    }
    const auto* body = reply.at(receipt_key);
    const auto signature = reply.stringAt("receipt_signature");
    if (body == nullptr || !signature) {
        error = "missing receipt or signature";
        return std::nullopt;
    }
    Receipt receipt;
    receipt.signature = encoding::base64Decode(*signature);
    if (body->isString()) {
        const auto& text = body->string();
        receipt.body_json.assign(text.begin(), text.end());
    } else {
        const auto dumped = json::pythonDumps(*body);
        receipt.body_json.assign(dumped.begin(), dumped.end());
    }
    return receipt;
}

std::vector<std::pair<std::string, std::string>> commonFields(const std::string& hw_token,
                                                              const std::string& arae) {
    // user_access_token is the AR-universe user token from the ar.graph login; the glasses'
    // hardware service expects the "ar" universe here.
    return {{"access_token", hw_token},
            {"user_access_token", arae},
            {"user_token_universe", "ar"},
            {"pair_protocol_version", "4"}};
}

std::optional<json::Value> postForm(std::string_view url,
                                    const std::vector<std::pair<std::string, std::string>>& fields,
                                    bool gzip_body, std::string& error) {
    const auto encoded = encoding::formUrlEncode(fields);
    if (httpDebug()) {
        debugRequest(url, fields);
    }
    const auto body =
        gzip_body ? encoding::gzip(view(encoded)) : Bytes(encoded.begin(), encoded.end());
    std::vector<std::pair<std::string, std::string>> headers{
        {"Content-Type", std::string(FormUrlEncoded)}};
    if (gzip_body) {
        headers.emplace_back("Content-Encoding", "gzip");
    }
    const auto reply = http::post(url, headers, body, UserAgent);
    if (httpDebug()) {
        debugReply("[reply]", reply.status, reply.body);
    }
    if (reply.status == 0) {
        error = "network error contacting Meta";
        return std::nullopt;
    }
    auto parsed = json::parse(toString(reply.body));
    if (!parsed) {
        error = "Meta returned a non-JSON body (HTTP " + std::to_string(reply.status) + ")";
        return std::nullopt;
    }
    if (reply.status >= 400) {
        if (const auto* err = parsed->at("error")) {
            error = err->stringAt("message").value_or("Meta HTTP " + std::to_string(reply.status));
        } else {
            error = "Meta HTTP " + std::to_string(reply.status);
        }
        return std::nullopt;
    }
    return parsed;
}

// Await one svc36 message of the given type; fills payload. Returns false on timeout or a status
// frame (which sets glasses_status).
bool awaitType(GlassesSession& session, std::uint16_t type, Bytes& payload,
               std::uint16_t& glasses_status, std::string& error) {
    const auto deadline = std::chrono::steady_clock::now() + StepTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        auto message = session.receiveRaw(remaining);
        if (!message) {
            error = "the glasses stopped answering";
            return false;
        }
        if (message->service != IdentityService && message->service != 0) {
            continue; // not identity traffic
        }
        if (message->has_error) {
            glasses_status = message->error;
            if (message->error != datax::StatusSuccess) {
                error = "the glasses rejected the step";
                return false;
            }
            continue;
        }
        if (message->type == type) {
            payload = std::move(message->payload);
            return true;
        }
    }
    error = "timed out waiting for the glasses";
    return false;
}

// One svc36 request/reply step of the change-owner exchange. On failure result.error carries the
// glasses' or transport's reason, or fallback_error when neither gave one.
bool identityStep(GlassesSession& session, std::uint16_t request_type, ByteView request,
                  std::uint16_t reply_type, Bytes& payload, ProvisionResult& result,
                  std::string_view fallback_error) {
    if (session.sendOnService(IdentityService, request_type, request) &&
        awaitType(session, reply_type, payload, result.glasses_status, result.error)) {
        return true;
    }
    if (result.error.empty()) {
        result.error = fallback_error;
    }
    return false;
}
} // namespace

Authorize authorizeRequest() {
    const auto state = randomState();
    const std::vector<std::pair<std::string, std::string>> query{
        {"app_id", std::string(AppId)},
        {"scope", "openid"},
        {"response_type", "code"},
        {"redirect_uri", std::string(RedirectUri)},
        {"state", state}};
    return {.url = std::string(AuthorizeUrl) + "?" + encoding::formUrlEncode(query),
            .state = state};
}

namespace {
std::optional<std::string> exchangeCode(const std::string& code, std::string& error) {
    const std::vector<std::pair<std::string, std::string>> fields{
        {"mt_app_id", std::string(AppId)},
        {"oidc_auth_code", code},
        {"redirect_uri", std::string(RedirectUri)}};
    const auto parsed = postForm(ExchangeUrl, fields, false, error);
    if (!parsed) {
        return std::nullopt;
    }
    auto token = parsed->stringAt("access_token");
    // user_id comes back as a JSON number, so require only its presence, not a string.
    if (!token || token->empty() || parsed->at("user_id") == nullptr) {
        error = "token-exchange returned no access_token/user_id (the code may be stale)";
        return std::nullopt;
    }
    return token; // the FRLA
}

// FRLA -> AR-universe user token via ar.graph.meta.com/login. The AR app token (caller-supplied,
// not embedded) is the OAuth bearer; the FRLA is the only body credential.
std::optional<std::string> arLogin(const std::string& frla, const std::string& ar_app_token,
                                   std::string& error) {
    const std::vector<std::pair<std::string, std::string>> fields{
        {"frl_access_token", frla},
        {"format", "json"},
        {"device_id", "meta-g-link-direct"},
        {"generate_session_cookies", "1"},
        {"generate_analytics_claim", "1"}};
    const auto encoded = encoding::formUrlEncode(fields);
    const std::vector<std::pair<std::string, std::string>> headers{
        {"Authorization", std::string("OAuth ") + ar_app_token},
        {"Content-Type", std::string(FormUrlEncoded)}};
    const auto reply = http::post(ArLoginUrl, headers, view(encoded), UserAgent);
    if (httpDebug()) {
        debugReply("[ar-login]", reply.status, reply.body);
    }
    if (reply.status == 0) {
        error = "network error contacting ar.graph";
        return std::nullopt;
    }
    const auto parsed = json::parse(toString(reply.body));
    if (!parsed) {
        error = "AR login returned a non-JSON body (HTTP " + std::to_string(reply.status) + ")";
        return std::nullopt;
    }
    if (const auto* err = parsed->at("error")) {
        error = err->stringAt("message").value_or("AR login error");
        return std::nullopt;
    }
    auto token = parsed->stringAt("access_token");
    if (!token || token->empty()) {
        error = "AR login returned no access_token";
        return std::nullopt;
    }
    return token; // the ARAe
}
} // namespace

AuthResult authenticate(const std::string& auth_code,
                        const std::vector<std::string>& ar_app_tokens) {
    AuthResult result;
    if (ar_app_tokens.empty()) {
        result.error = "no AR app token to log in with (expected an \"AR|<id>|<secret>\" token)";
        return result;
    }
    std::string error;
    const auto frla = exchangeCode(auth_code, error);
    if (!frla) {
        result.error = error;
        return result;
    }
    // One of the AR app tokens must be accepted by ar.graph; try each in order until the login
    // returns an ARAe. The last failure's reason is reported if none works.
    const auto accepted = std::ranges::find_if(ar_app_tokens, [&](const std::string& ar_app_token) {
        auto arae = arLogin(*frla, ar_app_token, error);
        if (arae) {
            result.token = std::move(*arae);
        }
        return arae.has_value();
    });
    if (accepted == ar_app_tokens.end()) {
        result.error = error.empty() ? "Meta accepted none of the AR app tokens" : error;
    }
    return result;
}

ProvisionResult runChangeOwner(GlassesSession& session, const crypto::OwnerKey& owner,
                               const std::string& hw_token, const std::string& user_token) {
    ProvisionResult result;
    Bytes payload;

    result.stage = ProvisionStage::Identity;
    if (!identityStep(session, MtIdentityRequest, {}, MtIdentityResponse, payload, result,
                      "could not ask the glasses who they are")) {
        return result;
    }
    const auto identity = proto::Message::parse(payload);
    if (!identity) {
        result.error = "unreadable identity response";
        return result;
    }
    const auto cert = identity->bytes(1);
    const auto serial_bytes = identity->bytes(2);
    if (!cert || !serial_bytes) {
        result.error = "identity response missing certificate or serial";
        return result;
    }
    const std::string serial = toString(*serial_bytes);

    result.stage = ProvisionStage::SkipChallenge;
    if (!identityStep(session, MtSkipChallenge, {}, MtSkipChallengeAck, payload, result,
                      "the glasses did not answer the skip-challenge")) {
        return result;
    }
    const auto challenge = proto::Message::parse(payload);
    const auto nonce = challenge ? challenge->bytes(1) : std::nullopt;
    if (!nonce) {
        result.error = "skip-challenge ack had no nonce";
        return result;
    }

    result.stage = ProvisionStage::PairRequest;
    json::Object additional;
    additional.emplace_back("device_nonce",
                            json::Value::makeString(encoding::base64Encode(*nonce)));
    additional.emplace_back("app_pubkey",
                            json::Value::makeString(encoding::base64Encode(
                                ByteView(owner.publicKey().data(), owner.publicKey().size()))));
    additional.emplace_back("secondary_cert", json::Value::makeString(""));
    auto pair_fields = commonFields(hw_token, user_token);
    pair_fields.emplace_back("device_cert", encoding::base64Encode(*cert));
    pair_fields.emplace_back("serial_number", serial);
    pair_fields.emplace_back("additional_data",
                             json::pythonDumps(json::Value::makeObject(std::move(additional))));
    auto pending_reply =
        postForm(GraphBase.data() + std::string("/pair_request"), pair_fields, false, result.error);
    if (!pending_reply) {
        return result;
    }
    const auto pending = parseReceipt(*pending_reply, "pending_ownership_receipt", result.error);
    if (!pending) {
        return result;
    }

    result.stage = ProvisionStage::StartChangeOwner;
    if (!identityStep(session, MtStartChangeOwnerReq, pending->encode(), MtStartChangeOwnerResp,
                      payload, result, "the glasses did not accept the pending receipt")) {
        return result;
    }
    const auto device_pending = proto::Message::parse(payload);
    const auto device_sig = device_pending ? device_pending->bytes(1) : std::nullopt;
    const auto device_body = device_pending ? device_pending->bytes(2) : std::nullopt;
    if (!device_sig || !device_body) {
        result.error = "device pending receipt was malformed";
        return result;
    }

    result.stage = ProvisionStage::Pair;
    auto final_fields = commonFields(hw_token, user_token);
    final_fields.emplace_back("device_pending_ownership_receipt", toString(*device_body));
    final_fields.emplace_back("device_pending_ownership_receipt_signature",
                              encoding::base64Encode(*device_sig));
    auto final_reply =
        postForm(GraphBase.data() + std::string("/pair"), final_fields, false, result.error);
    if (!final_reply) {
        return result;
    }
    const auto final_receipt = parseReceipt(*final_reply, "final_ownership_receipt", result.error);
    if (!final_receipt) {
        return result;
    }

    result.stage = ProvisionStage::FinishChangeOwner;
    if (!identityStep(session, MtFinishChangeOwnerReq, final_receipt->encode(),
                      MtFinishChangeOwnerResp, payload, result,
                      "the glasses did not acknowledge the final receipt")) {
        return result;
    }

    // The final receipt body names the serial and user id.
    if (const auto body = json::parse(toString(final_receipt->body_json))) {
        result.serial = body->stringAt("serial").value_or(serial);
        result.user_id = body->stringAt("user_id").value_or("");
    } else {
        result.serial = serial;
    }
    result.stage = ProvisionStage::Done;
    result.ok = true;
    return result;
}
} // namespace mgl::provision
