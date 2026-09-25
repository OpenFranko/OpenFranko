#include "StreetControls.h"

#include "../../street/StageFrame.h"

namespace openfranko::src::engine::states::level1 {

street::StreetInput
readStreetInput(const systems::ControllerSystem &controller) {
  street::StreetInput input;
  input.joystick = controller.joystick();
  if (const auto key = controller.functionKey()) {
    switch (*key) {
    case systems::FunctionKey::F1:
      input.key = street::SystemKey::MusicOn;
      break;
    case systems::FunctionKey::F2:
      input.key = street::SystemKey::MusicOff;
      break;
    case systems::FunctionKey::Escape:
      input.key = street::SystemKey::Escape;
      break;
    case systems::FunctionKey::F3:
      input.key = street::SystemKey::Pal;
      break;
    case systems::FunctionKey::F4:
      input.key = street::SystemKey::Ntsc;
      break;
    case systems::FunctionKey::Other:
      input.key = street::SystemKey::Other;
      break;
    }
  }
  return input;
}

void showStageFrame(systems::VideoSystem &videoSystem,
                    const systems::Display &frame,
                    const effects::GameOptions &options) {
  videoSystem.setNtsc(street::stageLayout(options).ntsc);
  videoSystem.show(frame);
}

} // namespace openfranko::src::engine::states::level1
