#include "StreetControls.h"

namespace openfranko::src::engine::states::level1 {
namespace {

constexpr auto MENU_MUSIC_PATH = "assets/0261.s3m";
constexpr int MENU_MUSIC_VOLUME = 63;

} // namespace

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

void restartMenuMusic(systems::AudioSystem &audio,
                      const effects::GameOptions &options) {
  audio.stopSFX();
  audio.loadMusic(MENU_MUSIC_PATH);
  audio.playMusic();
  audio.setMusicVolume(options.music ? MENU_MUSIC_VOLUME : 0);
}

} // namespace openfranko::src::engine::states::level1
