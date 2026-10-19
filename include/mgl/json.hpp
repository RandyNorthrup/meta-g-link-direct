// SPDX-License-Identifier: AGPL-3.0-only
// A tiny order-preserving JSON model. Only what the Meta setup calls need: parse a response, read
// string fields, and re-serialise an object exactly as Python's json.dumps does, because a Meta
// ownership receipt returned as an object must be rebuilt byte-for-byte to match its signature.
// The Python-dumps behaviour follows Visor core/identity/provision PyJson.kt at the revision in
// THIRD_PARTY_NOTICES.md.
#ifndef MGL_JSON_HPP
#define MGL_JSON_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mgl::json {
class Value;
using Object = std::vector<std::pair<std::string, Value>>;
using Array = std::vector<Value>;

enum class Kind : std::uint8_t { Null, Bool, Number, String, Object, Array };

class Value {
  public:
    Value() = default;
    static Value makeBool(bool value);
    static Value makeNumber(std::string raw); // kept as its source text
    static Value makeString(std::string value);
    static Value makeObject(Object value);
    static Value makeArray(Array value);

    [[nodiscard]] Kind kind() const noexcept { return kind_; }
    [[nodiscard]] bool isString() const noexcept { return kind_ == Kind::String; }
    [[nodiscard]] const std::string& string() const noexcept { return text_; }
    [[nodiscard]] const Object& object() const noexcept { return *object_; }
    [[nodiscard]] const Array& array() const noexcept { return *array_; }

    // For an object: the first string value for key, else nullopt.
    [[nodiscard]] std::optional<std::string> stringAt(std::string_view key) const;
    // For an object: the first value for key, else nullptr.
    [[nodiscard]] const Value* at(std::string_view key) const;

  private:
    Kind kind_ = Kind::Null;
    bool bool_ = false;
    std::string text_;
    std::shared_ptr<Object> object_;
    std::shared_ptr<Array> array_;
};

// nullopt on malformed input. The parser is strict and bounded to the bytes given, and rejects
// containers nested beyond a fixed depth limit (128).
[[nodiscard]] std::optional<Value> parse(std::string_view text);

// Python json.dumps(value) with its defaults: ", " and ": " separators, ensure_ascii escaping.
[[nodiscard]] std::string pythonDumps(const Value& value);
} // namespace mgl::json
#endif
