#ifndef TEST_SRC_ENGINE_STREET_SCENES_STAGERUNNER_H_
#define TEST_SRC_ENGINE_STREET_SCENES_STAGERUNNER_H_

#include "../../../../../src/engine/street/scenes/Stage.h"

#include <cstdint>
#include <functional>

namespace openfranko {
namespace test {
namespace src {
namespace engine {
namespace street {
namespace scenes {

template <typename Fixture> class StageRunner {
public:
  using SystemKey = openfranko::src::engine::street::session::SystemKey;

  void run(int frames, int16_t joystick = 0, SystemKey key = SystemKey::None) {
    for (int frame = 0; frame < frames; ++frame) {
      played().advance({joystick, frame == 0 ? key : SystemKey::None});
    }
  }

  int runUntil(const std::function<bool()> &done, int limit,
               int16_t joystick = 0) {
    for (int frame = 0; frame < limit; ++frame) {
      played().advance({joystick, SystemKey::None});
      if (done()) {
        return frame + 1;
      }
    }
    return -1;
  }

private:
  openfranko::src::engine::street::scenes::Stage &played() {
    return *static_cast<Fixture &>(*this).stage;
  }
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace test
} // namespace openfranko

#endif // TEST_SRC_ENGINE_STREET_SCENES_STAGERUNNER_H_
