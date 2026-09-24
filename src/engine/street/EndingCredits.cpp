#include "EndingCredits.h"

#include "Json.h"

#include <utility>

namespace openfranko::src::engine::street {

EndingCredits EndingCredits::fromJson(const std::string &json) {
  const JsonValue root = parseJson(json);
  EndingCredits credits;
  for (const JsonValue &record : root.member("pages").array()) {
    CreditPage page;
    page.beat = record.member("beat").integer();
    for (const JsonValue &line : record.member("lines").array()) {
      page.lines.push_back(
          {line.member("text").string(), line.member("y").integer()});
    }
    credits.pages.push_back(std::move(page));
  }
  return credits;
}

} // namespace openfranko::src::engine::street
