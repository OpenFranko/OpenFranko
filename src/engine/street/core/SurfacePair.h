#ifndef ENGINE_STREET_CORE_SURFACEPAIR_H_
#define ENGINE_STREET_CORE_SURFACEPAIR_H_

#include "../../amal/Machine.h"
#include "Bobs.h"
#include "IndexedSurface.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace core {

class SurfacePair {
public:
  SurfacePair(int width, int height);

  IndexedSurface &compose();
  void compose(const IndexedSurface &screen, const BobLayer &bobs,
               ImageBank &images);
  const IndexedSurface &shown() const;

private:
  bool isComposed(const IndexedSurface &screen, const BobLayer &bobs) const;

  std::array<IndexedSurface, 2> m_surfaces;
  std::size_t m_current = 0;
  mutable bool m_shown = false;
  const IndexedSurface *m_screen = nullptr;
  uint32_t m_revision = 0;
  std::array<uint32_t, BobLayer::MASK_WORDS> m_active{};
  std::array<amal::Object, BobLayer::BOBS> m_objects{};
};

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CORE_SURFACEPAIR_H_
