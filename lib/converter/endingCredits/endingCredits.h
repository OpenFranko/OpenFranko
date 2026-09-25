#ifndef ENDINGCREDITS_H_
#define ENDINGCREDITS_H_

#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace endingCredits {

struct Line {
  std::string text;
  int y = 0;
};

struct Page {
  std::vector<Line> lines;
  int beat = 0;
};

std::vector<std::vector<uint8_t>>
readHunks(const std::vector<uint8_t> &executable);
std::vector<Page> extract(const std::vector<uint8_t> &executable);
std::vector<uint8_t> toJson(const std::vector<Page> &pages);

} // namespace endingCredits
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // ENDINGCREDITS_H_
