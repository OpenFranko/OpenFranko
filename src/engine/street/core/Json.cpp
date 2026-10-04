#include "Json.h"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace openfranko::src::engine::street::core {
namespace {

constexpr int DECIMAL = 10;
constexpr std::size_t INTEGER_DIGITS = 9;
constexpr std::size_t OBJECT_MEMBERS = 16;
constexpr std::size_t ARRAY_ITEMS = 4;

bool isSpace(char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }

bool isDigit(char c) { return c >= '0' && c <= '9'; }

bool continuesNumber(char c) {
  return isDigit(c) || c == '.' || c == 'e' || c == 'E';
}

} // namespace

JsonCursor::JsonCursor(std::string text) : m_text(std::move(text)) {}

JsonValue JsonCursor::value() {
  const char c = peek();
  JsonValue result;
  if (c == '{') {
    result.kind = JsonValue::Kind::Object;
    result.members.reserve(OBJECT_MEMBERS);
    openObject();
    std::string key;
    while (nextMember(key)) {
      result.members.emplace_back(std::move(key), value());
    }
    return result;
  }
  if (c == '[') {
    result.kind = JsonValue::Kind::Array;
    result.items.reserve(ARRAY_ITEMS);
    openArray();
    while (nextItem()) {
      result.items.push_back(value());
    }
    return result;
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

void JsonCursor::skip() {
  const char c = peek();
  if (c == '{') {
    openObject();
    std::string key;
    while (nextMember(key)) {
      skip();
    }
    return;
  }
  if (c == '[') {
    openArray();
    while (nextItem()) {
      skip();
    }
    return;
  }
  if (c == '"') {
    skipString();
    return;
  }
  if ((c == 'n' && consume("null")) || (c == 't' && consume("true")) ||
      (c == 'f' && consume("false"))) {
    return;
  }
  number();
}

void JsonCursor::openObject() {
  expect('{');
  m_started.push_back(false);
}

bool JsonCursor::nextMember(std::string &key) {
  if (!next('}')) {
    return false;
  }
  key = string();
  expect(':');
  return true;
}

void JsonCursor::openArray() {
  expect('[');
  m_started.push_back(false);
}

bool JsonCursor::nextItem() { return next(']'); }

bool JsonCursor::skipNull() { return peek() == 'n' && consume("null"); }

int JsonCursor::integer() {
  const char c = peek();
  int whole = 0;
  if ((c == '-' || isDigit(c)) && wholeNumber(whole)) {
    return whole;
  }
  return value().integer();
}

std::string JsonCursor::text() {
  if (peek() != '"') {
    return value().string();
  }
  return string();
}

void JsonCursor::finish() {
  skipSpace();
  if (m_at != m_text.size()) {
    fail("trailing characters");
  }
}

void JsonCursor::fail(const std::string &reason) const {
  throw std::invalid_argument("JSON: " + reason + " at offset " +
                              std::to_string(m_at));
}

void JsonCursor::skipSpace() {
  const std::size_t size = m_text.size();
  while (m_at != size && isSpace(m_text[m_at])) {
    ++m_at;
  }
}

char JsonCursor::peek() {
  skipSpace();
  if (m_at == m_text.size()) {
    fail("unexpected end");
  }
  return m_text[m_at];
}

void JsonCursor::expect(char c) {
  if (peek() != c) {
    fail(std::string("expected '") + c + "'");
  }
  ++m_at;
}

bool JsonCursor::consume(std::string_view word) {
  if (m_text.size() - m_at >= word.size() &&
      std::string_view(m_text.data() + m_at, word.size()) == word) {
    m_at += word.size();
    return true;
  }
  return false;
}

bool JsonCursor::next(char close) {
  if (m_started.back()) {
    if (peek() != ',') {
      expect(close);
      m_started.pop_back();
      return false;
    }
    ++m_at;
    return true;
  }
  if (peek() == close) {
    ++m_at;
    m_started.pop_back();
    return false;
  }
  m_started.back() = true;
  return true;
}

bool JsonCursor::wholeNumber(int &whole) {
  const char *data = m_text.data();
  const std::size_t size = m_text.size();
  const std::size_t digits = data[m_at] == '-' ? m_at + 1 : m_at;
  std::size_t next = digits;
  long value = 0;
  while (next != size && next - digits < INTEGER_DIGITS &&
         isDigit(data[next])) {
    value = value * DECIMAL + (data[next] - '0');
    ++next;
  }
  if (next == digits || (next != size && continuesNumber(data[next]))) {
    return false;
  }
  whole = static_cast<int>(digits != m_at ? -value : value);
  m_at = next;
  return true;
}

double JsonCursor::number() {
  int whole = 0;
  if (wholeNumber(whole)) {
    return static_cast<double>(whole);
  }
  const char *start = m_text.data() + m_at;
  char *end = nullptr;
  const long parsedWhole = std::strtol(start, &end, DECIMAL);
  double parsed = static_cast<double>(parsedWhole);
  if (end == start || *end == '.' || *end == 'e' || *end == 'E' ||
      parsedWhole == LONG_MAX || parsedWhole == LONG_MIN) {
    parsed = std::strtod(start, &end);
  }
  if (end == start) {
    fail("unexpected character");
  }
  m_at = static_cast<std::size_t>(end - m_text.data());
  return parsed;
}

std::string JsonCursor::string() {
  expect('"');
  const std::size_t size = m_text.size();
  std::size_t end = m_at;
  while (end != size && m_text[end] != '"' && m_text[end] != '\\') {
    ++end;
  }
  if (end != size && m_text[end] == '"') {
    std::string text(m_text, m_at, end - m_at);
    m_at = end + 1;
    return text;
  }
  std::string text;
  while (m_at != size && m_text[m_at] != '"') {
    if (m_text[m_at] == '\\') {
      ++m_at;
      if (m_at == size) {
        break;
      }
    }
    text += m_text[m_at++];
  }
  expect('"');
  return text;
}

void JsonCursor::skipString() {
  expect('"');
  const std::size_t size = m_text.size();
  while (m_at != size && m_text[m_at] != '"') {
    if (m_text[m_at] == '\\') {
      ++m_at;
      if (m_at == size) {
        break;
      }
    }
    ++m_at;
  }
  expect('"');
}

const JsonValue &JsonValue::member(std::string_view key) const {
  if (kind == Kind::Object) {
    for (const auto &entry : members) {
      if (entry.first == key) {
        return entry.second;
      }
    }
  }
  missingMember(key);
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
  JsonCursor cursor(text);
  JsonValue root = cursor.value();
  cursor.finish();
  return root;
}

void missingMember(std::string_view key) {
  throw std::invalid_argument("JSON: missing \"" + std::string(key) + "\"");
}

} // namespace openfranko::src::engine::street::core
