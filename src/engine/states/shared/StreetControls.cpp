#include "StreetControls.h"

#include "../../street/ui/StageFrame.h"

namespace openfranko::src::engine::states::shared {

namespace {

street::ui::SystemKey
heldSystemKey(const systems::input::ControllerSystem &controller) {
  if (controller.isKeyHeld(systems::input::Key::Escape)) {
    return street::ui::SystemKey::Escape;
  }
  if (controller.isKeyHeld(systems::input::Key::F9)) {
    return street::ui::SystemKey::Lives;
  }
  if (controller.isKeyHeld(systems::input::Key::F1)) {
    return street::ui::SystemKey::Pal;
  }
  if (controller.isKeyHeld(systems::input::Key::F2)) {
    return street::ui::SystemKey::Ntsc;
  }
  if (controller.isKeyHeld(systems::input::Key::F3)) {
    return street::ui::SystemKey::MusicOff;
  }
  if (controller.isKeyHeld(systems::input::Key::F4)) {
    return street::ui::SystemKey::MusicOn;
  }
  return street::ui::SystemKey::None;
}

} // namespace

street::scenes::StreetInput
readStreetInput(const systems::input::ControllerSystem &controller,
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
    case systems::input::FunctionKey::F1:
      input.key = street::ui::SystemKey::MusicOn;
      break;
    case systems::input::FunctionKey::F2:
      input.key = street::ui::SystemKey::MusicOff;
      break;
    case systems::input::FunctionKey::Escape:
      input.key = street::ui::SystemKey::Escape;
      break;
    case systems::input::FunctionKey::F3:
      input.key = street::ui::SystemKey::Pal;
      break;
    case systems::input::FunctionKey::F4:
      input.key = street::ui::SystemKey::Ntsc;
      break;
    case systems::input::FunctionKey::Other:
      input.key = street::ui::SystemKey::Other;
      break;
    }
  }
  return input;
}

void showStageFrame(systems::graphics::VideoSystem &videoSystem,
                    const systems::graphics::Display &frame,
                    const effects::core::GameOptions &options) {
  videoSystem.setNtsc(street::ui::stageLayout(options).ntsc);
  videoSystem.show(frame);
}

} // namespace openfranko::src::engine::states::shared
