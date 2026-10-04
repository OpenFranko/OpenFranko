#ifndef ENGINE_STREET_CORE_ENDINGCREDITS_H_
#define ENGINE_STREET_CORE_ENDINGCREDITS_H_

#include "Json.h"

#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace core {

struct CreditLine {
  std::string text;
  int y = 0;
};

struct CreditPage {
  std::vector<CreditLine> lines;
  int beat = 0;
};

struct EndingCredits {
  std::vector<CreditPage> pages;

  static EndingCredits fromJson(const std::string &json);
};

class EndingCreditsReader {
public:
  explicit EndingCreditsReader(std::string json);

  bool step(EndingCredits &credits);

private:
  JsonCursor m_json;
  bool m_opened = false;
  bool m_inPages = false;
  bool m_hasPages = false;
};

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CORE_ENDINGCREDITS_H_
