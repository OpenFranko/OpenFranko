#ifndef SYSTEMS_GRAPHICS_PLANARFRAME_H_
#define SYSTEMS_GRAPHICS_PLANARFRAME_H_

#include "graphics/IndexedRasterizer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {

class PlanarFrame {
public:
  static constexpr int PLANES = 4;

  void resize(int width, int height);
  void copy(const IndexedFrame &frame, int row, Span span);
  void copyColumns(const IndexedFrame &frame, Span columns);
  void shift(int pixels);
  const uint8_t *line(int plane, int row) const;
  int bytes() const;
  int pitch() const;

private:
  struct View {
    std::size_t buffer = 0;
    int base = 0;
  };

  uint8_t *start(int plane, int row);
  void recenter(View &view, int moved);

  std::array<std::vector<uint8_t>, PLANES> m_buffers;
  std::array<View, PLANES> m_views{};
  int m_bytes = 0;
  int m_pitch = 0;
  int m_rows = 0;
};

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_PLANARFRAME_H_
