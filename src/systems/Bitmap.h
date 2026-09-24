#ifndef SYSTEMS_BITMAP_H_
#define SYSTEMS_BITMAP_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {

struct IndexedBitmap {
  int width = 0;
  int height = 0;
  int hotspotX = 0;
  int hotspotY = 0;
  std::vector<uint8_t> pixels;
  std::vector<uint16_t> palette;
};

int channelToNibble(uint8_t channel);
std::pair<int, int> readBitmapHotspot(const std::string &path);
IndexedBitmap loadIndexedBitmap(const std::string &path);

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_BITMAP_H_
