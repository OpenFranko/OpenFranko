#include "Json.h"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace openfranko::src::engine::street::core {
namespace {

constexpr int DECIMAL = 10;
constexpr std::size_t OBJECT_MEMBERS = 16;
constexpr std::size_t ARRAY_ITEMS = 4;

bool isSpace(char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }

class JsonReader {
public:
  explicit JsonReader(const std::string &text) : m_text(text) {}

  JsonValue document() {
    JsonValue root = value();
    skipSpace();
    if (m_position != m_text.size()) {
      fail("trailing characters");
    }
    return root;
  }

private:
  [[noreturn]] void fail(const std::string &reason) const {
    throw std::invalid_argument("JSON: " + reason + " at offset " +
                                std::to_string(m_position));
  }

  void skipSpace() {
    while (m_position < m_text.size() && isSpace(m_text[m_position])) {
      ++m_position;
    }
  }

  char peek() {
    skipSpace();
    if (m_position >= m_text.size()) {
      fail("unexpected end");
    }
    return m_text[m_position];
  }

  void expect(char c) {
    if (peek() != c) {
      fail(std::string("expected '") + c + "'");
    }
    ++m_position;
  }

  bool consume(std::string_view word) {
    if (m_text.compare(m_position, word.size(), word) == 0) {
      m_position += word.size();
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
      ++m_position;
      if (peek() == '}') {
        ++m_position;
        return result;
      }
      for (;;) {
        std::string key = string();
        expect(':');
        result.members.emplace_back(std::move(key), value());
        if (peek() == ',') {
          ++m_position;
          continue;
        }
        expect('}');
        return result;
      }
    }
    if (c == '[') {
      result.kind = JsonValue::Kind::Array;
      result.items.reserve(ARRAY_ITEMS);
      ++m_position;
      if (peek() == ']') {
        ++m_position;
        return result;
      }
      for (;;) {
        result.items.push_back(value());
        if (peek() == ',') {
          ++m_position;
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
    if (consume("null")) {
      return result;
    }
    if (consume("true")) {
      result.kind = JsonValue::Kind::Boolean;
      result.boolean = true;
      return result;
    }
    if (consume("false")) {
      result.kind = JsonValue::Kind::Boolean;
      return result;
    }
    const char *start = m_text.c_str() + m_position;
    char *end = nullptr;
    const long whole = std::strtol(start, &end, DECIMAL);
    if (end == start || *end == '.' || *end == 'e' || *end == 'E' ||
        whole == LONG_MAX || whole == LONG_MIN) {
      result.number = std::strtod(start, &end);
    } else {
      result.number = static_cast<double>(whole);
    }
    if (end == start) {
      fail("unexpected character");
    }
    result.kind = JsonValue::Kind::Number;
    m_position += static_cast<std::size_t>(end - start);
    return result;
  }

  std::string string() {
    expect('"');
    const std::size_t end = m_text.find_first_of("\"\\", m_position);
    if (end != std::string::npos && m_text[end] == '"') {
      std::string text = m_text.substr(m_position, end - m_position);
      m_position = end + 1;
      return text;
    }
    std::string text;
    while (m_position < m_text.size() && m_text[m_position] != '"') {
      if (m_text[m_position] == '\\') {
        ++m_position;
        if (m_position >= m_text.size()) {
          break;
        }
      }
      text += m_text[m_position++];
    }
    expect('"');
    return text;
  }

  const std::string &m_text;
  std::size_t m_position = 0;
};

} // namespace

const JsonValue &JsonValue::member(const std::string &key) const {
  if (kind == Kind::Object) {
    for (const auto &entry : members) {
      if (entry.first == key) {
        return entry.second;
      }
    }
  }
  throw std::invalid_argument("JSON: missing \"" + key + "\"");
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
