#ifndef BITMAPEXTRACTOR_H_
#define BITMAPEXTRACTOR_H_

#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace bitmapExtractor {

struct ExtractedBitmap {
  std::string name;
  std::vector<uint8_t> bmpData;
  std::string error;
};

std::vector<ExtractedBitmap> extract(const std::vector<uint8_t> &data,
                                     const std::string &fileId);

} // namespace bitmapExtractor
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // BITMAPEXTRACTOR_H_
