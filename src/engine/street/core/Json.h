#ifndef ENGINE_STREET_CORE_JSON_H_
#define ENGINE_STREET_CORE_JSON_H_

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

JsonValue parseJson(const std::string &text);

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CORE_JSON_H_
