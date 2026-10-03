#include "SurfacePair.h"

namespace openfranko::src::engine::street::core {

SurfacePair::SurfacePair(int width, int height)
    : m_surfaces{IndexedSurface(width, height), IndexedSurface(width, height)} {
}

IndexedSurface &SurfacePair::compose() {
  if (m_shown) {
    m_current ^= 1;
    m_shown = false;
  }
  return m_surfaces[m_current];
}

const IndexedSurface &SurfacePair::shown() const {
  m_shown = true;
  return m_surfaces[m_current];
}

} // namespace openfranko::src::engine::street::core
