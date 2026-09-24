#include "StreetControls.h"

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
    case systems::FunctionKey::F4:
      break;
    }
  }
  return input;
}

} // namespace openfranko::src::engine::states::level1
