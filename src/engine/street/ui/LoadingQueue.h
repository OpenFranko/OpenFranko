#ifndef ENGINE_STREET_UI_LOADINGQUEUE_H_
#define ENGINE_STREET_UI_LOADINGQUEUE_H_

#include "StatusPanel.h"

#include <deque>
#include <functional>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace ui {

class LoadingQueue {
public:
  static constexpr int READ_FRAMES = 25;
  static constexpr int UNPACK_FRAMES = 25;
  static constexpr int FILE_FRAMES = READ_FRAMES + UNPACK_FRAMES;

  void queue(std::function<void()> load);
  bool advance(StatusPanel *panel);

private:
  enum class Phase { Idle, Reading, Unpacking };

  std::deque<std::function<void()>> m_files;
  Phase m_phase = Phase::Idle;
  int m_countdown = 0;
};

} // namespace ui
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_UI_LOADINGQUEUE_H_
