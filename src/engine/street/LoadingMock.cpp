#include "LoadingMock.h"

#include <utility>

namespace openfranko::src::engine::street {

void LoadingMock::queue(std::function<void()> load) {
  m_files.push_back(std::move(load));
}

bool LoadingMock::advance(StatusPanel &panel) {
  if (m_phase != Phase::Idle && --m_countdown > 0) {
    return false;
  }
  if (m_phase == Phase::Reading) {
    panel.showWaiting();
    m_phase = Phase::Unpacking;
    m_countdown = UNPACK_FRAMES;
    return false;
  }
  m_phase = Phase::Idle;
  if (m_files.empty()) {
    return true;
  }
  std::function<void()> load = std::move(m_files.front());
  m_files.pop_front();
  load();
  panel.showLoading();
  m_phase = Phase::Reading;
  m_countdown = READ_FRAMES;
  return false;
}

} // namespace openfranko::src::engine::street
