// SPDX-License-Identifier: AGPL-3.0-only
// pythonDumps mirrors Visor core/identity/provision PyJson.kt at the revision in
// THIRD_PARTY_NOTICES.md.
#include "mgl/json.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace mgl::json {
namespace {
constexpr int HexBase = 16;
constexpr unsigned PrintableLow = 0x20;
constexpr unsigned PrintableHigh = 0x7E;

struct Parser {
    std::string_view text;
    std::size_t pos = 0;
    bool failed = false;

    void skipSpace() {
        while (pos < text.size() &&
               (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n' || text[pos] == '\r')) {
            ++pos;
        }
    }

    char peek() { return pos < text.size() ? text[pos] : '\0'; }

    bool consume(char expected) {
        if (peek() == expected) {
            ++pos;
            return true;
        }
        failed = true;
        return false;
    }

    std::optional<Value> parseValue();
    std::optional<std::string> parseStringRaw();
    std::optional<Value> parseNumber();
};

std::optional<std::string> Parser::parseStringRaw() {
    if (!consume('"')) {
        return std::nullopt;
    }
    std::string out;
    while (pos < text.size()) {
        const char ch = text[pos++];
        if (ch == '"') {
            return out;
        }
        if (ch != '\\') {
            out.push_back(ch);
            continue;
        }
        if (pos >= text.size()) {
            break;
        }
        const char escape = text[pos++];
        switch (escape) {
        case '"':
            out.push_back('"');
            break;
        case '\\':
            out.push_back('\\');
            break;
        case '/':
            out.push_back('/');
            break;
        case 'n':
            out.push_back('\n');
            break;
        case 't':
            out.push_back('\t');
            break;
        case 'r':
            out.push_back('\r');
            break;
        case 'b':
            out.push_back('\b');
            break;
        case 'f':
            out.push_back('\f');
            break;
        case 'u': {
            if (text.size() - pos < 4) {
                failed = true;
                return std::nullopt;
            }
            unsigned code = 0;
            for (int index = 0; index < 4; ++index) {
                const char digit = text[pos++];
                code *= HexBase;
                if (digit >= '0' && digit <= '9') {
                    code += static_cast<unsigned>(digit - '0');
                } else if (digit >= 'a' && digit <= 'f') {
                    code += static_cast<unsigned>(digit - 'a' + 10);
                } else if (digit >= 'A' && digit <= 'F') {
                    code += static_cast<unsigned>(digit - 'A' + 10);
                } else {
                    failed = true;
                    return std::nullopt;
                }
            }
            // Encode the code point as UTF-8 (surrogate halves pass through as-is; the Meta
            // responses this reads are ASCII, so this path is defensive only).
            if (code < 0x80) {
                out.push_back(static_cast<char>(code));
            } else if (code < 0x800) {
                out.push_back(static_cast<char>(0xC0 | (code >> 6)));
                out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            } else {
                out.push_back(static_cast<char>(0xE0 | (code >> 12)));
                out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
            }
            break;
        }
        default:
            failed = true;
            return std::nullopt;
        }
    }
    failed = true;
    return std::nullopt;
}

std::optional<Value> Parser::parseNumber() {
    const auto start = pos;
    if (peek() == '-') {
        ++pos;
    }
    while (pos < text.size()) {
        const char ch = text[pos];
        if ((ch >= '0' && ch <= '9') || ch == '.' || ch == 'e' || ch == 'E' || ch == '+' ||
            ch == '-') {
            ++pos;
        } else {
            break;
        }
    }
    if (pos == start) {
        failed = true;
        return std::nullopt;
    }
    return Value::makeNumber(std::string(text.substr(start, pos - start)));
}

std::optional<Value> Parser::parseValue() {
    skipSpace();
    const char ch = peek();
    if (ch == '"') {
        auto str = parseStringRaw();
        if (!str) {
            return std::nullopt;
        }
        return Value::makeString(std::move(*str));
    }
    if (ch == '{') {
        ++pos;
        Object object;
        skipSpace();
        if (peek() == '}') {
            ++pos;
            return Value::makeObject(std::move(object));
        }
        for (;;) {
            skipSpace();
            auto key = parseStringRaw();
            if (!key) {
                return std::nullopt;
            }
            skipSpace();
            if (!consume(':')) {
                return std::nullopt;
            }
            auto value = parseValue();
            if (!value) {
                return std::nullopt;
            }
            object.emplace_back(std::move(*key), std::move(*value));
            skipSpace();
            if (peek() == ',') {
                ++pos;
                continue;
            }
            if (consume('}')) {
                return Value::makeObject(std::move(object));
            }
            return std::nullopt;
        }
    }
    if (ch == '[') {
        ++pos;
        Array array;
        skipSpace();
        if (peek() == ']') {
            ++pos;
            return Value::makeArray(std::move(array));
        }
        for (;;) {
            auto value = parseValue();
            if (!value) {
                return std::nullopt;
            }
            array.push_back(std::move(*value));
            skipSpace();
            if (peek() == ',') {
                ++pos;
                continue;
            }
            if (consume(']')) {
                return Value::makeArray(std::move(array));
            }
            return std::nullopt;
        }
    }
    if (ch == 't') {
        if (text.substr(pos, 4) == "true") {
            pos += 4;
            return Value::makeBool(true);
        }
        return std::nullopt;
    }
    if (ch == 'f') {
        if (text.substr(pos, 5) == "false") {
            pos += 5;
            return Value::makeBool(false);
        }
        return std::nullopt;
    }
    if (ch == 'n') {
        if (text.substr(pos, 4) == "null") {
            pos += 4;
            return Value();
        }
        return std::nullopt;
    }
    return parseNumber();
}

void dumpString(std::string_view value, std::string& out) {
    out.push_back('"');
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        default:
            if (ch < PrintableLow || ch > PrintableHigh) {
                std::array<char, 7> buffer{};
                const auto* const hex = "0123456789abcdef";
                buffer[0] = '\\';
                buffer[1] = 'u';
                buffer[2] = '0';
                buffer[3] = '0';
                buffer[4] = hex[(ch >> 4) & 0xF];
                buffer[5] = hex[ch & 0xF];
                out.append(buffer.data(), 6);
            } else {
                out.push_back(static_cast<char>(ch));
            }
            break;
        }
    }
    out.push_back('"');
}

void dumpValue(const Value& value, std::string& out) {
    switch (value.kind()) {
    case Kind::Null:
        out += "null";
        break;
    case Kind::Bool:
        out += value.string(); // stored as "true"/"false"
        break;
    case Kind::Number:
        out += value.string();
        break;
    case Kind::String:
        dumpString(value.string(), out);
        break;
    case Kind::Object: {
        out.push_back('{');
        bool first = true;
        for (const auto& [key, child] : value.object()) {
            if (!first) {
                out += ", ";
            }
            first = false;
            dumpString(key, out);
            out += ": ";
            dumpValue(child, out);
        }
        out.push_back('}');
        break;
    }
    case Kind::Array: {
        out.push_back('[');
        bool first = true;
        for (const auto& child : value.array()) {
            if (!first) {
                out += ", ";
            }
            first = false;
            dumpValue(child, out);
        }
        out.push_back(']');
        break;
    }
    }
}
} // namespace

Value Value::makeBool(bool value) {
    Value out;
    out.kind_ = Kind::Bool;
    out.bool_ = value;
    out.text_ = value ? "true" : "false";
    return out;
}

Value Value::makeNumber(std::string raw) {
    Value out;
    out.kind_ = Kind::Number;
    out.text_ = std::move(raw);
    return out;
}

Value Value::makeString(std::string value) {
    Value out;
    out.kind_ = Kind::String;
    out.text_ = std::move(value);
    return out;
}

Value Value::makeObject(Object value) {
    Value out;
    out.kind_ = Kind::Object;
    out.object_ = std::make_shared<Object>(std::move(value));
    return out;
}

Value Value::makeArray(Array value) {
    Value out;
    out.kind_ = Kind::Array;
    out.array_ = std::make_shared<Array>(std::move(value));
    return out;
}

const Value* Value::at(std::string_view key) const {
    if (kind_ != Kind::Object) {
        return nullptr;
    }
    for (const auto& [name, value] : *object_) {
        if (name == key) {
            return &value;
        }
    }
    return nullptr;
}

std::optional<std::string> Value::stringAt(std::string_view key) const {
    const auto* found = at(key);
    if (found == nullptr || !found->isString()) {
        return std::nullopt;
    }
    return found->string();
}

std::optional<Value> parse(std::string_view text) {
    Parser parser{.text = text};
    auto value = parser.parseValue();
    if (parser.failed || !value) {
        return std::nullopt;
    }
    parser.skipSpace();
    if (parser.pos != text.size()) {
        return std::nullopt;
    }
    return value;
}

std::string pythonDumps(const Value& value) {
    std::string out;
    dumpValue(value, out);
    return out;
}
} // namespace mgl::json
