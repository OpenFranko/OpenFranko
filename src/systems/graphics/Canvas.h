#ifndef SYSTEMS_GRAPHICS_CANVAS_H_
#define SYSTEMS_GRAPHICS_CANVAS_H_

#include "graphics/Bitmap.h"
#include "graphics/Display.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {

class Canvas {
public:
  static constexpr uint8_t FILL_INDEX = 255;

  Canvas();
  Canvas(int width, int height);
  Canvas(const Canvas &other);
  Canvas(Canvas &&other) = default;
  Canvas &operator=(const Canvas &other);
  Canvas &operator=(Canvas &&other) = default;
  ~Canvas() = default;

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
  Display output(const std::vector<Sprite> &sprites) const;

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
  uint32_t m_revision = newRevision();
  mutable bool m_shown = false;
};

std::optional<Sprite> spriteOf(const IndexedBitmap &image, int x, int y);

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_CANVAS_H_
