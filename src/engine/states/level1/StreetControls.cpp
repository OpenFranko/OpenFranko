#include "StreetControls.h"

#include "../../street/StageFrame.h"

namespace openfranko::src::engine::states::level1 {

namespace {

street::SystemKey heldSystemKey(const systems::ControllerSystem &controller) {
  if (controller.isKeyHeld(systems::Key::Escape)) {
    return street::SystemKey::Escape;
  }
  if (controller.isKeyHeld(systems::Key::F9)) {
    return street::SystemKey::Lives;
  }
  if (controller.isKeyHeld(systems::Key::F1)) {
    return street::SystemKey::Pal;
  }
  if (controller.isKeyHeld(systems::Key::F2)) {
    return street::SystemKey::Ntsc;
  }
  if (controller.isKeyHeld(systems::Key::F3)) {
    return street::SystemKey::MusicOff;
  }
  if (controller.isKeyHeld(systems::Key::F4)) {
    return street::SystemKey::MusicOn;
  }
  return street::SystemKey::None;
}

} // namespace

street::StreetInput readStreetInput(const systems::ControllerSystem &controller,
                                    GameVersion version) {
  street::StreetInput input;
  input.joystick = controller.joystick();
  if (version == GameVersion::V12) {
    input.key = heldSystemKey(controller);
    input.mouseButton = controller.isMouseButtonDown();
    return input;
  }
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
