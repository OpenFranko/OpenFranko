#include "Level1State.h"

#include "StreetControls.h"

namespace openfranko::src::engine::states::level1 {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "level1Frame";

} // namespace

Level1State::Level1State(systems::VideoSystem &videoSystem,
                         systems::AudioSystem &audioSystem,
                         systems::ControllerSystem &controllerSystem,
                         effects::GameOptions &options,
                         street::GameSession &session)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_controllerSystem(controllerSystem), m_options(options),
      m_host(audioSystem), m_stage(m_host, session, options) {
  m_videoSystem.createScreen(SCREEN, street::FRAME_WIDTH, street::FRAME_HEIGHT);
  m_videoSystem.switchScreen(SCREEN);
}

Level1State::~Level1State() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> Level1State::update() {
  m_stage.advance(readStreetInput(m_controllerSystem));
  m_stage.compose(m_frame);
  m_videoSystem.updateFrameImage(FRAME, street::FRAME_WIDTH,
                                 street::FRAME_HEIGHT, m_frame);
  m_videoSystem.drawImage(FRAME, 0, 0);

  switch (m_stage.outcome()) {
  case street::StreetStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::StreetStage::Outcome::Quit:
    restartMenuMusic(m_audioSystem, m_options);
    return EngineStateEnum::Menu;
  case street::StreetStage::Outcome::LevelFinished:
    return EngineStateEnum::Level1Boss;
  case street::StreetStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::StreetStage &Level1State::stage() const { return m_stage; }

} // namespace openfranko::src::engine::states::level1
