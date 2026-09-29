#include "StreetControls.h"

namespace openfranko::src::engine::states::shared {
namespace {

street::session::SystemKey
heldSystemKey(const systems::input::ControllerSystem &controller) {
  if (controller.isKeyHeld(systems::input::Key::Escape)) {
    return street::session::SystemKey::Escape;
  }
  if (controller.isKeyHeld(systems::input::Key::F9)) {
    return street::session::SystemKey::Lives;
  }
  if (controller.isKeyHeld(systems::input::Key::F1)) {
    return street::session::SystemKey::Pal;
  }
  if (controller.isKeyHeld(systems::input::Key::F2)) {
    return street::session::SystemKey::Ntsc;
  }
  if (controller.isKeyHeld(systems::input::Key::F3)) {
    return street::session::SystemKey::MusicOff;
  }
  if (controller.isKeyHeld(systems::input::Key::F4)) {
    return street::session::SystemKey::MusicOn;
  }
  return street::session::SystemKey::None;
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
      input.key = street::session::SystemKey::MusicOn;
      break;
    case systems::input::FunctionKey::F2:
      input.key = street::session::SystemKey::MusicOff;
      break;
    case systems::input::FunctionKey::Escape:
      input.key = street::session::SystemKey::Escape;
      break;
    case systems::input::FunctionKey::F3:
      input.key = street::session::SystemKey::Pal;
      break;
    case systems::input::FunctionKey::F4:
      input.key = street::session::SystemKey::Ntsc;
      break;
    case systems::input::FunctionKey::Other:
      input.key = street::session::SystemKey::Other;
      break;
    }
  }
  return input;
}

} // namespace openfranko::src::engine::states::shared
