#ifndef SYSTEMS_GRAPHICS_BITMAP_H_
#define SYSTEMS_GRAPHICS_BITMAP_H_

#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {

struct IndexedBitmap {
  int width = 0;
  int height = 0;
  int hotspotX = 0;
  int hotspotY = 0;
  std::vector<uint8_t> pixels;
  std::vector<uint16_t> palette;
};

IndexedBitmap readIndexedBitmap(const std::vector<uint8_t> &file);
IndexedBitmap loadIndexedBitmap(const std::string &path);

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_BITMAP_H_
