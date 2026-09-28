#ifndef ENGINE_STREET_ENDINGCREDITS_H_
#define ENGINE_STREET_ENDINGCREDITS_H_

#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

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

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_ENDINGCREDITS_H_
