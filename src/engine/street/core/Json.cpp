#include "Json.h"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace openfranko::src::engine::street::core {
namespace {

constexpr int DECIMAL = 10;
constexpr int INTEGER_DIGITS = 9;
constexpr std::size_t OBJECT_MEMBERS = 16;
constexpr std::size_t ARRAY_ITEMS = 4;

bool isSpace(char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }

bool isDigit(char c) { return c >= '0' && c <= '9'; }

bool continuesNumber(char c) {
  return isDigit(c) || c == '.' || c == 'e' || c == 'E';
}

class JsonReader {
public:
  explicit JsonReader(const std::string &text)
      : m_start(text.c_str()), m_cursor(m_start), m_end(m_start + text.size()) {
  }

  JsonValue document() {
    JsonValue root = value();
    skipSpace();
    if (m_cursor != m_end) {
      fail("trailing characters");
    }
    return root;
  }

private:
  [[noreturn]] void fail(const std::string &reason) const {
    throw std::invalid_argument(
        "JSON: " + reason + " at offset " +
        std::to_string(static_cast<std::size_t>(m_cursor - m_start)));
  }

  void skipSpace() {
    while (m_cursor != m_end && isSpace(*m_cursor)) {
      ++m_cursor;
    }
  }

  char peek() {
    skipSpace();
    if (m_cursor == m_end) {
      fail("unexpected end");
    }
    return *m_cursor;
  }

  void expect(char c) {
    if (peek() != c) {
      fail(std::string("expected '") + c + "'");
    }
    ++m_cursor;
  }

  bool consume(std::string_view word) {
    if (static_cast<std::size_t>(m_end - m_cursor) >= word.size() &&
        std::string_view(m_cursor, word.size()) == word) {
      m_cursor += word.size();
      return true;
    }
    return false;
  }

  JsonValue value() {
    const char c = peek();
    JsonValue result;
    if (c == '{') {
      result.kind = JsonValue::Kind::Object;
      result.members.reserve(OBJECT_MEMBERS);
      ++m_cursor;
      if (peek() == '}') {
        ++m_cursor;
        return result;
      }
      for (;;) {
        std::string key = string();
        expect(':');
        result.members.emplace_back(std::move(key), value());
        if (peek() == ',') {
          ++m_cursor;
          continue;
        }
        expect('}');
        return result;
      }
    }
    if (c == '[') {
      result.kind = JsonValue::Kind::Array;
      result.items.reserve(ARRAY_ITEMS);
      ++m_cursor;
      if (peek() == ']') {
        ++m_cursor;
        return result;
      }
      for (;;) {
        result.items.push_back(value());
        if (peek() == ',') {
          ++m_cursor;
          continue;
        }
        expect(']');
        return result;
      }
    }
    if (c == '"') {
      result.kind = JsonValue::Kind::String;
      result.text = string();
      return result;
    }
    if (c == 'n' && consume("null")) {
      return result;
    }
    if (c == 't' && consume("true")) {
      result.kind = JsonValue::Kind::Boolean;
      result.boolean = true;
      return result;
    }
    if (c == 'f' && consume("false")) {
      result.kind = JsonValue::Kind::Boolean;
      return result;
    }
    result.number = number();
    result.kind = JsonValue::Kind::Number;
    return result;
  }

  double number() {
    const bool negative = *m_cursor == '-';
    const char *digits = negative ? m_cursor + 1 : m_cursor;
    const char *next = digits;
    long whole = 0;
    while (next != m_end && next - digits < INTEGER_DIGITS && isDigit(*next)) {
      whole = whole * DECIMAL + (*next - '0');
      ++next;
    }
    if (next != digits && (next == m_end || !continuesNumber(*next))) {
      m_cursor = next;
      return static_cast<double>(negative ? -whole : whole);
    }
    char *end = nullptr;
    whole = std::strtol(m_cursor, &end, DECIMAL);
    double parsed = static_cast<double>(whole);
    if (end == m_cursor || *end == '.' || *end == 'e' || *end == 'E' ||
        whole == LONG_MAX || whole == LONG_MIN) {
      parsed = std::strtod(m_cursor, &end);
    }
    if (end == m_cursor) {
      fail("unexpected character");
    }
    m_cursor = end;
    return parsed;
  }

  std::string string() {
    expect('"');
    const char *end = m_cursor;
    while (end != m_end && *end != '"' && *end != '\\') {
      ++end;
    }
    if (end != m_end && *end == '"') {
      std::string text(m_cursor, end);
      m_cursor = end + 1;
      return text;
    }
    std::string text;
    while (m_cursor != m_end && *m_cursor != '"') {
      if (*m_cursor == '\\') {
        ++m_cursor;
        if (m_cursor == m_end) {
          break;
        }
      }
      text += *m_cursor++;
    }
    expect('"');
    return text;
  }

  const char *m_start;
  const char *m_cursor;
  const char *m_end;
};

} // namespace

const JsonValue &JsonValue::member(std::string_view key) const {
  if (kind == Kind::Object) {
    for (const auto &entry : members) {
      if (entry.first == key) {
        return entry.second;
      }
    }
  }
  throw std::invalid_argument("JSON: missing \"" + std::string(key) + "\"");
}

int JsonValue::integer() const {
  if (kind != Kind::Number || std::floor(number) != number) {
    throw std::invalid_argument("JSON: expected an integer");
  }
  return static_cast<int>(number);
}

const std::string &JsonValue::string() const {
  if (kind != Kind::String) {
    throw std::invalid_argument("JSON: expected a string");
  }
  return text;
}

const std::vector<JsonValue> &JsonValue::array() const {
  if (kind != Kind::Array) {
    throw std::invalid_argument("JSON: expected an array");
  }
  return items;
}

JsonValue parseJson(const std::string &text) {
  return JsonReader(text).document();
}

} // namespace openfranko::src::engine::street::core
