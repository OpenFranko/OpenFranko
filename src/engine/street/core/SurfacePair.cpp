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

void SurfacePair::compose(const IndexedSurface &screen, const BobLayer &bobs,
                          ImageBank &images) {
  if (isComposed(screen, bobs)) {
    return;
  }
  IndexedSurface &display = compose();
  display = screen;
  bobs.draw(display, images);
  m_screen = &screen;
  m_revision = screen.revision();
  for (int word = 0; word < BobLayer::MASK_WORDS; ++word) {
    m_active[static_cast<std::size_t>(word)] = bobs.activeBits(word);
  }
  for (int number = 0; number < BobLayer::BOBS; ++number) {
    m_objects[static_cast<std::size_t>(number)] = bobs.placedObject(number);
  }
}

bool SurfacePair::isComposed(const IndexedSurface &screen,
                             const BobLayer &bobs) const {
  if (&screen != m_screen || screen.revision() != m_revision) {
    return false;
  }
  for (int word = 0; word < BobLayer::MASK_WORDS; ++word) {
    if (bobs.activeBits(word) != m_active[static_cast<std::size_t>(word)]) {
      return false;
    }
  }
  for (int number = 0; number < BobLayer::BOBS; ++number) {
    if (!bobs.isActive(number)) {
      continue;
    }
    const amal::Object &placed = bobs.placedObject(number);
    const amal::Object &composed = m_objects[static_cast<std::size_t>(number)];
    if (placed.x != composed.x || placed.y != composed.y ||
        placed.image != composed.image) {
      return false;
    }
  }
  return true;
}

const IndexedSurface &SurfacePair::shown() const {
  m_shown = true;
  return m_surfaces[m_current];
}

} // namespace openfranko::src::engine::street::core
