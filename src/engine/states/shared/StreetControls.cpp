#include "StreetControls.h"

#include "../../street/core/StageFrame.h"

namespace openfranko::src::engine::states::shared {

namespace {

street::ui::SystemKey
heldSystemKey(const systems::ControllerSystem &controller) {
  if (controller.isKeyHeld(systems::Key::Escape)) {
    return street::ui::SystemKey::Escape;
  }
  if (controller.isKeyHeld(systems::Key::F9)) {
    return street::ui::SystemKey::Lives;
  }
  if (controller.isKeyHeld(systems::Key::F1)) {
    return street::ui::SystemKey::Pal;
  }
  if (controller.isKeyHeld(systems::Key::F2)) {
    return street::ui::SystemKey::Ntsc;
  }
  if (controller.isKeyHeld(systems::Key::F3)) {
    return street::ui::SystemKey::MusicOff;
  }
  if (controller.isKeyHeld(systems::Key::F4)) {
    return street::ui::SystemKey::MusicOn;
  }
  return street::ui::SystemKey::None;
}

} // namespace

street::scenes::StreetInput
readStreetInput(const systems::ControllerSystem &controller,
                GameVersion version) {
  street::scenes::StreetInput input;
  input.joystick = controller.joystick();
  if (version == GameVersion::V12) {
    input.key = heldSystemKey(controller);
    input.mouseButton = controller.isMouseButtonDown();
    return input;
  }
  if (const auto key = controller.functionKey()) {
    switch (*key) {
    case systems::FunctionKey::F1:
      input.key = street::ui::SystemKey::MusicOn;
      break;
    case systems::FunctionKey::F2:
      input.key = street::ui::SystemKey::MusicOff;
      break;
    case systems::FunctionKey::Escape:
      input.key = street::ui::SystemKey::Escape;
      break;
    case systems::FunctionKey::F3:
      input.key = street::ui::SystemKey::Pal;
      break;
    case systems::FunctionKey::F4:
      input.key = street::ui::SystemKey::Ntsc;
      break;
    case systems::FunctionKey::Other:
      input.key = street::ui::SystemKey::Other;
      break;
    }
  }
  return input;
}

void showStageFrame(systems::VideoSystem &videoSystem,
                    const systems::Display &frame,
                    const effects::core::GameOptions &options) {
  videoSystem.setNtsc(street::core::stageLayout(options).ntsc);
  videoSystem.show(frame);
}

} // namespace openfranko::src::engine::states::shared
