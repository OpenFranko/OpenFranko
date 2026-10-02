#include "LoadingQueue.h"

#include <utility>

namespace openfranko::src::engine::street::ui {

void LoadingQueue::queue(std::function<void()> load) {
  m_files.push_back(Job{std::move(load), nullptr});
}

void LoadingQueue::queueSteps(std::function<bool()> step) {
  m_files.push_back(Job{nullptr, std::move(step)});
}

bool LoadingQueue::advance(StatusPanel *panel) {
  if (m_running && m_running()) {
    m_running = nullptr;
  }
  if (m_phase != Phase::Idle && --m_countdown > 0) {
    return false;
  }
  if (m_phase == Phase::Reading) {
    if (panel) {
      panel->showWaiting();
    }
    m_phase = Phase::Unpacking;
    m_countdown = UNPACK_FRAMES;
    return false;
  }
  if (m_running) {
    m_countdown = 1;
    return false;
  }
  m_phase = Phase::Idle;
  if (m_files.empty()) {
    return true;
  }
  Job job = std::move(m_files.front());
  m_files.pop_front();
  if (job.load) {
    job.load();
  } else if (!job.step()) {
    m_running = std::move(job.step);
  }
  if (panel) {
    panel->showLoading();
  }
  m_phase = Phase::Reading;
  m_countdown = READ_FRAMES;
  return false;
}

} // namespace openfranko::src::engine::street::ui
