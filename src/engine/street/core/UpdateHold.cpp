#include "UpdateHold.h"

namespace openfranko::src::engine::street::core {

void UpdateHold::start(long frame, int frames) {
  m_start = frame;
  m_until = frame + frames;
}

bool UpdateHold::holdsAtStart(long frame) const {
  return m_start < frame && frame <= m_until;
}

bool UpdateHold::holdsAtEnd(long frame) const {
  return m_start <= frame && frame < m_until;
}

} // namespace openfranko::src::engine::street::core
