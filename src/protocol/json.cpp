// SPDX-License-Identifier: AGPL-3.0-only
// pythonDumps mirrors Visor core/identity/provision PyJson.kt at the revision in
// THIRD_PARTY_NOTICES.md.
#include "mgl/json.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mgl::json {
namespace {
constexpr unsigned HexBase = 16;
constexpr std::size_t HexEscapeDigits = 4;
constexpr unsigned NibbleBits = 4;
constexpr unsigned NibbleMask = 0x0FU;
constexpr unsigned PrintableLow = 0x20;
constexpr unsigned PrintableHigh = 0x7E;
// Bounds parser recursion. The Meta responses read here nest only a few levels.
constexpr std::size_t MaxNestingDepth = 128;
constexpr std::string_view Whitespace = " \t\n\r";
// Numbers are kept as their source text: the run of characters a JSON number can contain.
constexpr std::string_view NumberCharacters = "0123456789.eE+-";

std::optional<unsigned> hexDigitValue(char digit) {
    if (digit >= '0' && digit <= '9') {
        return static_cast<unsigned>(digit - '0');
    }
    if (digit >= 'a' && digit <= 'f') {
        return static_cast<unsigned>(digit - 'a' + 10);
    }
    if (digit >= 'A' && digit <= 'F') {
        return static_cast<unsigned>(digit - 'A' + 10);
    }
    return std::nullopt;
}

// Encodes a \u escape's code point as UTF-8 (surrogate halves pass through as-is; the Meta
// responses this reads are ASCII, so this path is defensive only).
void appendUtf8(unsigned code, std::string& out) {
    if (code < 0x80U) {
        out.push_back(static_cast<char>(code));
    } else if (code < 0x800U) {
        out.push_back(static_cast<char>(0xC0U | (code >> 6U)));
        out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
    } else {
        out.push_back(static_cast<char>(0xE0U | (code >> 12U)));
        out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
    }
}

struct Parser {
    std::string_view text;
    std::size_t pos = 0;
    bool failed = false;

    void skipSpace() { pos = std::min(text.find_first_not_of(Whitespace, pos), text.size()); }

    [[nodiscard]] char peek() const { return pos < text.size() ? text.at(pos) : '\0'; }

    bool consume(char expected) {
        if (peek() == expected) {
            ++pos;
            return true;
        }
        failed = true;
        return false;
    }

    bool consumeWord(std::string_view word) {
        if (!text.substr(pos).starts_with(word)) {
            return false;
        }
        pos += word.size();
        return true;
    }

    std::optional<Value> parseValue(std::size_t depth);
    std::optional<Value> parseObject(std::size_t depth);
    std::optional<Value> parseArray(std::size_t depth);
    std::optional<Value> parseLiteral();
    std::optional<Value> parseNumber();
    std::optional<std::string> parseStringRaw();
    bool parseEscape(std::string& out);
    std::optional<unsigned> parseHexEscape();
};

std::optional<unsigned> Parser::parseHexEscape() {
    if (text.size() - pos < HexEscapeDigits) {
        failed = true;
        return std::nullopt;
    }
    unsigned code = 0;
    for (const char digit : text.substr(pos, HexEscapeDigits)) {
        const auto value = hexDigitValue(digit);
        if (!value) {
            failed = true;
            return std::nullopt;
        }
        code = (code * HexBase) + *value;
    }
    pos += HexEscapeDigits;
    return code;
}

// Appends the character the escape after a backslash denotes; false if it is malformed.
bool Parser::parseEscape(std::string& out) {
    if (pos >= text.size()) {
        failed = true;
        return false;
    }
    const char escape = text.at(pos++);
    switch (escape) {
    case '"':
    case '\\':
    case '/':
        out.push_back(escape);
        return true;
    case 'n':
        out.push_back('\n');
        return true;
    case 't':
        out.push_back('\t');
        return true;
    case 'r':
        out.push_back('\r');
        return true;
    case 'b':
        out.push_back('\b');
        return true;
    case 'f':
        out.push_back('\f');
        return true;
    case 'u': {
        const auto code = parseHexEscape();
        if (!code) {
            return false;
        }
        appendUtf8(*code, out);
        return true;
    }
    default:
        failed = true;
        return false;
    }
}

std::optional<std::string> Parser::parseStringRaw() {
    if (!consume('"')) {
        return std::nullopt;
    }
    std::string out;
    while (pos < text.size()) {
        const char ch = text.at(pos++);
        if (ch == '"') {
            return out;
        }
        if (ch != '\\') {
            out.push_back(ch);
        } else if (!parseEscape(out)) {
            return std::nullopt;
        }
    }
    failed = true;
    return std::nullopt;
}

std::optional<Value> Parser::parseNumber() {
    const auto start = pos;
    pos = std::min(text.find_first_not_of(NumberCharacters, pos), text.size());
    if (pos == start) {
        failed = true;
        return std::nullopt;
    }
    return Value::makeNumber(std::string(text.substr(start, pos - start)));
}

std::optional<Value> Parser::parseLiteral() {
    if (consumeWord("true")) {
        return Value::makeBool(true);
    }
    if (consumeWord("false")) {
        return Value::makeBool(false);
    }
    if (consumeWord("null")) {
        return Value();
    }
    return std::nullopt;
}

// parseValue, parseObject and parseArray recurse once per nested container; parseValue rejects
// nesting beyond MaxNestingDepth, which bounds the recursion.
std::optional<Value> Parser::parseObject(std::size_t depth) { // NOLINT(misc-no-recursion)
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
        auto value = parseValue(depth + 1);
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

std::optional<Value> Parser::parseArray(std::size_t depth) { // NOLINT(misc-no-recursion)
    Array array;
    skipSpace();
    if (peek() == ']') {
        ++pos;
        return Value::makeArray(std::move(array));
    }
    for (;;) {
        auto value = parseValue(depth + 1);
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

// depth is the number of containers enclosing the value.
std::optional<Value> Parser::parseValue(std::size_t depth) { // NOLINT(misc-no-recursion)
    skipSpace();
    const char ch = peek();
    if (ch == '{' || ch == '[') {
        if (depth >= MaxNestingDepth) {
            failed = true;
            return std::nullopt;
        }
        ++pos;
        return ch == '{' ? parseObject(depth) : parseArray(depth);
    }
    if (ch == '"') {
        auto str = parseStringRaw();
        if (!str) {
            return std::nullopt;
        }
        return Value::makeString(std::move(*str));
    }
    if (ch == 't' || ch == 'f' || ch == 'n') {
        return parseLiteral();
    }
    return parseNumber();
}

void dumpString(std::string_view value, std::string& out) {
    static constexpr std::string_view Hex = "0123456789abcdef";
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
                out += "\\u00";
                out.push_back(Hex.at(static_cast<unsigned>(ch) >> NibbleBits));
                out.push_back(Hex.at(static_cast<unsigned>(ch) & NibbleMask));
            } else {
                out.push_back(static_cast<char>(ch));
            }
            break;
        }
    }
    out.push_back('"');
}

// An object or array whose children are being written, and the index of its next child.
struct OpenContainer {
    const Value* container = nullptr;
    std::size_t next = 0;
};

// Writes what precedes the next child (separator, and the key for an object) and returns that
// child, or writes the closing bracket and returns nullptr when no children remain.
const Value* nextChild(OpenContainer& open, std::string& out) {
    const bool is_object = open.container->kind() == Kind::Object;
    const auto count = is_object ? open.container->object().size() : open.container->array().size();
    if (open.next == count) {
        out.push_back(is_object ? '}' : ']');
        return nullptr;
    }
    if (open.next != 0) {
        out += ", ";
    }
    const auto index = open.next++;
    if (!is_object) {
        return &open.container->array().at(index);
    }
    const auto& [key, child] = open.container->object().at(index);
    dumpString(key, out);
    out += ": ";
    return &child;
}

// Iterative, so a deeply nested value cannot exhaust the call stack.
void dumpValue(const Value& root, std::string& out) {
    std::vector<OpenContainer> open;
    const Value* next = &root;
    while (next != nullptr) {
        switch (next->kind()) {
        case Kind::Null:
            out += "null";
            break;
        case Kind::Bool: // stored as "true"/"false"
        case Kind::Number:
            out += next->string();
            break;
        case Kind::String:
            dumpString(next->string(), out);
            break;
        case Kind::Object:
            out.push_back('{');
            open.push_back({.container = next});
            break;
        case Kind::Array:
            out.push_back('[');
            open.push_back({.container = next});
            break;
        }
        next = nullptr;
        while (next == nullptr && !open.empty()) {
            next = nextChild(open.back(), out);
            if (next == nullptr) {
                open.pop_back();
            }
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
    auto value = parser.parseValue(0);
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
