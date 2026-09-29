#ifndef TEST_SRC_ENGINE_STREET_SCENES_SCENERUNNER_H_
#define TEST_SRC_ENGINE_STREET_SCENES_SCENERUNNER_H_

#include <cstdint>
#include <functional>

namespace openfranko {
namespace test {
namespace src {
namespace engine {
namespace street {
namespace scenes {

template <typename Fixture> class SceneRunner {
public:
  void run(int frames, int16_t joystick = 0) {
    for (int frame = 0; frame < frames; ++frame) {
      played().advance(joystick);
    }
  }

  int runUntil(const std::function<bool()> &done, int limit,
               int16_t joystick = 0) {
    for (int frame = 0; frame < limit; ++frame) {
      played().advance(joystick);
      if (done()) {
        return frame + 1;
      }
    }
    return -1;
  }

private:
  auto &played() { return static_cast<Fixture &>(*this).scene; }
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace test
} // namespace openfranko

#endif // TEST_SRC_ENGINE_STREET_SCENES_SCENERUNNER_H_
