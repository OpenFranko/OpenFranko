#include "EndingCredits.h"

#include "../core/Json.h"

#include <utility>

namespace openfranko::src::engine::street::scenes {

EndingCredits EndingCredits::fromJson(const std::string &json) {
  const core::JsonValue root = core::parseJson(json);
  EndingCredits credits;
  for (const core::JsonValue &record : root.member("pages").array()) {
    CreditPage page;
    page.beat = record.member("beat").integer();
    for (const core::JsonValue &line : record.member("lines").array()) {
      page.lines.push_back(
          {line.member("text").string(), line.member("y").integer()});
    }
    credits.pages.push_back(std::move(page));
  }
  return credits;
}

} // namespace openfranko::src::engine::street::scenes
