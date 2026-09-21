#include "WorldSoftwareState.h"

namespace openfranko::src::engine::states {

WorldSoftwareState::WorldSoftwareState(systems::VideoSystem &videoSystem)
    : m_videoSystem(videoSystem) {
  m_videoSystem.createScreen(0, 368, 290);
  m_videoSystem.switchScreen(0);
  m_videoSystem.loadImage("background", "assets/03B6.bmp");
};

WorldSoftwareState::~WorldSoftwareState() {
  m_videoSystem.clearImage("background");
  m_videoSystem.fillScreen(0, 0, 0);
};

std::optional<EngineStateEnum> WorldSoftwareState::update() {
  m_videoSystem.drawImage("background", 0, 0);
  return std::nullopt;
}

} // namespace openfranko::src::engine::states