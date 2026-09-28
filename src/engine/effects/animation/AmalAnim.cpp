#include "AmalAnim.h"

#include <algorithm>
#include <utility>

namespace openfranko::src::engine::effects {

AmalAnim::AmalAnim(std::vector<Frame> frames, int loops)
    : m_frames(std::move(frames)), m_loops(loops),
      m_finished(m_frames.empty()) {}

int AmalAnim::advance(int image) {
  if (m_finished) {
    return image;
  }
  if (!m_started) {
    m_started = true;
    m_wait = 1;
  }

  if (--m_wait > 0) {
    return image;
  }
  if (m_index >= m_frames.size()) {
    if (m_loops != 0 && --m_loops == 0) {
      m_finished = true;
      return image;
    }
    m_index = 0;
  }
  m_wait = std::max(1, m_frames[m_index].frames);
  return m_frames[m_index++].image;
}

bool AmalAnim::isFinished() const { return m_finished; }

} // namespace openfranko::src::engine::effects
