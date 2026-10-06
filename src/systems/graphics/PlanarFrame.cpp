#include "graphics/PlanarFrame.h"

#include <algorithm>
#include <cstring>

namespace openfranko::src::systems::graphics {
namespace {

constexpr int PLANE_MASK = PlanarFrame::PLANES - 1;
constexpr int MARGIN = 32;

} // namespace

void PlanarFrame::resize(int width, int height) {
  m_bytes = std::max(width, 0) / PLANES;
  m_pitch = m_bytes + 2 * MARGIN;
  m_rows = std::max(height, 0);
  for (std::size_t plane = 0; plane < m_buffers.size(); ++plane) {
    m_buffers[plane].resize(static_cast<std::size_t>(m_pitch) *
                            static_cast<std::size_t>(m_rows));
    m_views[plane] = {plane, MARGIN};
  }
}

void PlanarFrame::copy(const IndexedFrame &frame, int row, Span span) {
  const int first = std::max(span.first, 0);
  const int last = std::min(span.last, m_bytes * PLANES);
  if (first >= last) {
    return;
  }
  const uint8_t *in =
      frame.pixels.data() + static_cast<std::ptrdiff_t>(row) * frame.width;
  const std::array<uint8_t *, PLANES> out = {start(0, row), start(1, row),
                                             start(2, row), start(3, row)};
  const auto copyPixel = [&](int x) {
    out[static_cast<std::size_t>(x & PLANE_MASK)][x / PLANES] = in[x];
  };
  int x = first;
  for (; x < last && (x & PLANE_MASK) != 0; ++x) {
    copyPixel(x);
  }
  const int groups = (last - x) / PLANES;
  const int end = x / PLANES + groups;
  const uint8_t *group = in + end * PLANES;
  uint8_t *plane0 = out[0] + end;
  uint8_t *plane1 = out[1] + end;
  uint8_t *plane2 = out[2] + end;
  uint8_t *plane3 = out[3] + end;
  for (int step = -groups; step != 0; ++step) {
    plane0[step] = group[PLANES * step];
    plane1[step] = group[PLANES * step + 1];
    plane2[step] = group[PLANES * step + 2];
    plane3[step] = group[PLANES * step + 3];
  }
  for (x += groups * PLANES; x < last; ++x) {
    copyPixel(x);
  }
}

void PlanarFrame::copyColumns(const IndexedFrame &frame, Span columns) {
  const int begin = std::max(columns.first, 0);
  const int end = std::min(columns.last, m_bytes * PLANES);
  const auto stride = static_cast<std::ptrdiff_t>(frame.width);
  for (int x = begin; x < end; ++x) {
    uint8_t *out = start(x & PLANE_MASK, 0) + x / PLANES;
    const uint8_t *in = frame.pixels.data() + x;
    for (int row = 0; row < m_rows; ++row) {
      *out = *in;
      out += m_pitch;
      in += stride;
    }
  }
}

void PlanarFrame::shift(int pixels) {
  std::array<View, PLANES> shifted{};
  for (int plane = 0; plane < PLANES; ++plane) {
    const int from = plane - pixels;
    const int source = from & PLANE_MASK;
    const int moved = (from - source) / PLANES;
    View view = m_views[static_cast<std::size_t>(source)];
    if (view.base + moved < 0 || view.base + moved > m_pitch - m_bytes) {
      recenter(view, moved);
    } else {
      view.base += moved;
    }
    shifted[static_cast<std::size_t>(plane)] = view;
  }
  m_views = shifted;
}

const uint8_t *PlanarFrame::line(int plane, int row) const {
  const View &view = m_views[static_cast<std::size_t>(plane)];
  return m_buffers[view.buffer].data() +
         static_cast<std::ptrdiff_t>(row) * m_pitch + view.base;
}

int PlanarFrame::bytes() const { return m_bytes; }

int PlanarFrame::pitch() const { return m_pitch; }

uint8_t *PlanarFrame::start(int plane, int row) {
  const View &view = m_views[static_cast<std::size_t>(plane)];
  return m_buffers[view.buffer].data() +
         static_cast<std::ptrdiff_t>(row) * m_pitch + view.base;
}

void PlanarFrame::recenter(View &view, int moved) {
  const int base = moved > 0 ? 0 : m_pitch - m_bytes;
  const int first = std::clamp(-moved, 0, m_bytes);
  const int last = std::clamp(m_bytes - moved, first, m_bytes);
  uint8_t *buffer = m_buffers[view.buffer].data();
  for (int row = 0; first < last && row < m_rows; ++row) {
    uint8_t *line = buffer + static_cast<std::ptrdiff_t>(row) * m_pitch;
    std::memmove(line + base + first, line + view.base + first + moved,
                 static_cast<std::size_t>(last - first));
  }
  view.base = base;
}

} // namespace openfranko::src::systems::graphics
