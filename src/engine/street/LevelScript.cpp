#include "LevelScript.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace openfranko::src::engine::street {
namespace {

struct JsonValue {
  enum class Kind { Null, Boolean, Number, String, Array, Object };

  Kind kind = Kind::Null;
  bool boolean = false;
  double number = 0;
  std::string text;
  std::vector<JsonValue> items;
  std::vector<std::pair<std::string, JsonValue>> members;

  const JsonValue &member(const std::string &key) const {
    if (kind == Kind::Object) {
      for (const auto &entry : members) {
        if (entry.first == key) {
          return entry.second;
        }
      }
    }
    throw std::invalid_argument("Level script: missing \"" + key + "\"");
  }

  int integer() const {
    if (kind != Kind::Number || std::floor(number) != number) {
      throw std::invalid_argument("Level script: expected an integer");
    }
    return static_cast<int>(number);
  }

  const std::vector<JsonValue> &array() const {
    if (kind != Kind::Array) {
      throw std::invalid_argument("Level script: expected an array");
    }
    return items;
  }
};

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
    throw std::invalid_argument("Level script: " + reason + " at offset " +
                                std::to_string(m_position));
  }

  void skipSpace() {
    while (m_position < m_text.size() &&
           std::isspace(static_cast<unsigned char>(m_text[m_position]))) {
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

  bool consume(const char *word) {
    const std::string expected(word);
    if (m_text.compare(m_position, expected.size(), expected) == 0) {
      m_position += expected.size();
      return true;
    }
    return false;
  }

  JsonValue value() {
    const char c = peek();
    JsonValue result;
    if (c == '{') {
      result.kind = JsonValue::Kind::Object;
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
    result.number = std::strtod(start, &end);
    if (end == start) {
      fail("unexpected character");
    }
    result.kind = JsonValue::Kind::Number;
    m_position += static_cast<std::size_t>(end - start);
    return result;
  }

  std::string string() {
    expect('"');
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

LevelScript LevelScript::fromJson(const std::string &json) {
  const JsonValue root = JsonReader(json).document();
  LevelScript script;
  script.length = root.member("lengthInColumns").integer();
  for (const JsonValue &record : root.member("waves").array()) {
    Wave wave;
    wave.trigger = record.member("triggerColumn").integer();
    const auto &slots = record.member("slots").array();
    if (slots.size() != wave.slots.size()) {
      throw std::invalid_argument("Level script: a wave needs three slots");
    }
    for (std::size_t i = 0; i < slots.size(); ++i) {
      if (slots[i].kind == JsonValue::Kind::Null) {
        continue;
      }
      EnemySlot &slot = wave.slots[i];
      slot.spriteSet = slots[i].member("spriteSetId").integer();
      slot.type = slots[i].member("kind").integer();
      slot.x = slots[i].member("spawnX").integer();
      slot.y = slots[i].member("spawnY").integer();
      slot.energy = slots[i].member("energy").integer();
      slot.aggression = slots[i].member("aggression").integer();
    }
    script.waves.push_back(wave);
  }
  return script;
}

} // namespace openfranko::src::engine::street
