#ifndef ENGINE_STREET_CORE_SURFACEPAIR_H_
#define ENGINE_STREET_CORE_SURFACEPAIR_H_

#include "IndexedSurface.h"

#include <array>
#include <cstddef>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace core {

class SurfacePair {
public:
  SurfacePair(int width, int height);

  IndexedSurface &compose();
  const IndexedSurface &shown() const;

private:
  std::array<IndexedSurface, 2> m_surfaces;
  std::size_t m_current = 0;
  mutable bool m_shown = false;
};

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CORE_SURFACEPAIR_H_
