#ifndef ENGINE_STREET_CORE_JSON_H_
#define ENGINE_STREET_CORE_JSON_H_

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace core {

struct JsonValue {
  enum class Kind { Null, Boolean, Number, String, Array, Object };

  Kind kind = Kind::Null;
  bool boolean = false;
  double number = 0;
  std::string text;
  std::vector<JsonValue> items;
  std::vector<std::pair<std::string, JsonValue>> members;

  const JsonValue &member(std::string_view key) const;
  int integer() const;
  const std::string &string() const;
  const std::vector<JsonValue> &array() const;
};

class JsonCursor {
public:
  explicit JsonCursor(std::string text);

  JsonValue value();
  void skip();
  void openObject();
  bool nextMember(std::string &key);
  void openArray();
  bool nextItem();
  bool skipNull();
  int integer();
  std::string text();
  void finish();

private:
  [[noreturn]] void fail(const std::string &reason) const;
  void skipSpace();
  char peek();
  void expect(char c);
  bool consume(std::string_view word);
  bool next(char close);
  bool wholeNumber(int &whole);
  double number();
  std::string string();
  void skipString();

  std::string m_text;
  std::size_t m_at = 0;
  std::vector<bool> m_started;
};

JsonValue parseJson(const std::string &text);

[[noreturn]] void missingMember(std::string_view key);

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CORE_JSON_H_
