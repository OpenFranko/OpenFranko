#include "Level2State.h"

#include "../level1/StreetControls.h"

namespace openfranko::src::engine::states::level2 {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "level2Frame";

} // namespace

Level2State::Level2State(systems::VideoSystem &videoSystem,
                         systems::AudioSystem &audioSystem,
                         systems::ControllerSystem &controllerSystem,
                         effects::GameOptions &options,
                         street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem), m_stage(m_host, session, options) {
  m_videoSystem.createScreen(SCREEN, street::FRAME_WIDTH, street::FRAME_HEIGHT);
  m_videoSystem.switchScreen(SCREEN);
}

Level2State::~Level2State() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> Level2State::update() {
  m_stage.advance(level1::readStreetInput(m_controllerSystem));
  m_stage.compose(m_frame);
  m_videoSystem.updateFrameImage(FRAME, street::FRAME_WIDTH,
                                 street::FRAME_HEIGHT, m_frame);
  m_videoSystem.drawImage(FRAME, 0, 0);

  switch (m_stage.outcome()) {
  case street::StreetStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::StreetStage::Outcome::Quit:
    return EngineStateEnum::HighScore;
  case street::StreetStage::Outcome::LevelFinished:
    return EngineStateEnum::Level2Boss;
  case street::StreetStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::StreetStage &Level2State::stage() const { return m_stage; }

} // namespace openfranko::src::engine::states::level2
