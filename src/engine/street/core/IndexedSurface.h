#ifndef ENGINE_STREET_INDEXEDSURFACE_H_
#define ENGINE_STREET_INDEXEDSURFACE_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

struct Picture {
  int width = 0;
  int height = 0;
  int hotX = 0;
  int hotY = 0;
  std::vector<uint8_t> pixels;

  uint8_t at(int x, int y) const {
    return pixels[static_cast<std::size_t>(y * width + x)];
  }
};

class IndexedSurface {
public:
  IndexedSurface(int width, int height);

  int width() const;
  int height() const;
  uint8_t pixel(int x, int y) const;
  const std::vector<uint8_t> &pixels() const;

  void fill(uint8_t color);
  void clear(uint8_t color, int x1, int y1, int x2, int y2);
  void copy(const IndexedSurface &source, int x1, int y1, int x2, int y2, int x,
            int y);
  void unpack(const Picture &picture, int x, int y);
  bool intersects(int left, int top, int width, int height) const;
  void draw(const Picture &picture, int left, int top, bool flipX, bool flipY,
            bool opaque = false);

private:
  int m_width;
  int m_height;
  std::vector<uint8_t> m_pixels;
};

class ScreenBlock {
public:
  ScreenBlock(const IndexedSurface &source, int x, int y, int width,
              int height);

  void put(IndexedSurface &target) const;

private:
  IndexedSurface m_pixels;
  int m_x;
  int m_y;
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_INDEXEDSURFACE_H_
