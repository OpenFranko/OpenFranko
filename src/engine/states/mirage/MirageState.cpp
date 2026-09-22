#include "MirageState.h"

namespace openfranko::src::engine::states::mirage {

MirageState::MirageState(systems::VideoSystem &videoSystem,
                         systems::ControllerSystem &controllerSystem)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem) {
  videoSystem.createScreen(0, 368, 290);
  videoSystem.switchScreen(0);
  m_videoSystem.loadImage("background", "assets/03C3.bmp");
};

MirageState::~MirageState() {
  m_videoSystem.clearImage("background");
  m_videoSystem.fillScreen(0, 0, 0);
};

std::optional<EngineStateEnum> MirageState::update() {
  m_videoSystem.drawImage("background", 0, 0);
  if (m_controllerSystem.states.button) {
    return EngineStateEnum::WorldSoftware;
  }
  return std::nullopt;
}

} // namespace openfranko::src::engine::states::mirage