#ifndef SYSTEMS_GRAPHICS_CANVAS_H_
#define SYSTEMS_GRAPHICS_CANVAS_H_

#include "graphics/Bitmap.h"
#include "graphics/Display.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {

class Canvas {
public:
  static constexpr uint8_t FILL_INDEX = 255;

  Canvas() = default;
  Canvas(int width, int height);

  int width() const;
  int height() const;
  const std::vector<uint8_t> &pixels() const;
  const std::vector<uint16_t> &palette() const;

  void fill(uint16_t color);
  void setPalette(const std::vector<uint16_t> &colors);
  void draw(const IndexedBitmap &image, int x, int y);
  void drawMasked(const IndexedBitmap &image, int x, int y,
                  bool flipped = false);
  Display output() const;

private:
  bool covers(const IndexedBitmap &image, int x, int y) const;
  void prepare(bool covered);
  void blit(const IndexedBitmap &image, int x, int y, bool masked,
            bool flipped);

  int m_width = 0;
  int m_height = 0;
  std::vector<uint8_t> m_pixels;
  std::vector<uint8_t> m_spare;
  std::vector<uint16_t> m_palette = std::vector<uint16_t>(256, 0);
  mutable bool m_shown = false;
};

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_CANVAS_H_
