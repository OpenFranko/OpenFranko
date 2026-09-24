#include "StreetControls.h"

#include "../../street/StageFrame.h"

namespace openfranko::src::engine::states::level1 {
namespace {

constexpr int STAGE_SCREEN = 0;

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
      input.key = street::SystemKey::Pal;
      break;
    case systems::FunctionKey::F4:
      input.key = street::SystemKey::Ntsc;
      break;
    }
  }
  return input;
}

void openStageScreen(systems::VideoSystem &videoSystem,
                     const effects::GameOptions &options) {
  const street::StageLayout layout = street::stageLayout(options);
  videoSystem.setNtsc(layout.ntsc);
  videoSystem.createScreen(STAGE_SCREEN, street::FRAME_WIDTH,
                           street::frameRows(layout), street::FRAME_HEIGHT);
  videoSystem.switchScreen(STAGE_SCREEN);
}

void showStageFrame(systems::VideoSystem &videoSystem, const std::string &name,
                    const std::vector<uint32_t> &frame,
                    const effects::GameOptions &options) {
  const street::StageLayout layout = street::stageLayout(options);
  videoSystem.setNtsc(layout.ntsc);
  videoSystem.updateFrameImage(name, street::FRAME_WIDTH,
                               street::frameRows(layout), frame);
  videoSystem.drawImage(name, 0, 0);
}

} // namespace openfranko::src::engine::states::level1
