#ifndef SYSTEMS_CANVAS_H_
#define SYSTEMS_CANVAS_H_

#include "Bitmap.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {

class Canvas {
public:
  Canvas() = default;
  Canvas(int width, int height);

  int width() const;
  int height() const;
  const std::vector<uint32_t> &pixels() const;

  void fill(uint16_t color);
  void draw(const IndexedBitmap &image, const std::vector<uint16_t> &palette,
            int x, int y);
  void drawMasked(const IndexedBitmap &image,
                  const std::vector<uint16_t> &palette, int x, int y,
                  bool flipped = false);

private:
  void blit(const IndexedBitmap &image, const std::vector<uint16_t> &palette,
            int x, int y, bool masked, bool flipped);

  int m_width = 0;
  int m_height = 0;
  std::vector<uint32_t> m_pixels;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_CANVAS_H_
