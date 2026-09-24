#include "Level1State.h"

namespace openfranko::src::engine::states::level1 {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "level1Frame";
constexpr auto MENU_MUSIC_PATH = "assets/0261.s3m";
constexpr int MENU_MUSIC_VOLUME = 63;

} // namespace

Level1State::Level1State(systems::VideoSystem &videoSystem,
                         systems::AudioSystem &audioSystem,
                         systems::ControllerSystem &controllerSystem,
                         effects::GameOptions &options,
                         street::GameSession &session)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_controllerSystem(controllerSystem), m_options(options),
      m_host(audioSystem), m_stage(m_host, session, options) {
  m_videoSystem.createScreen(SCREEN, street::StreetStage::FRAME_WIDTH,
                             street::StreetStage::FRAME_HEIGHT);
  m_videoSystem.switchScreen(SCREEN);
}

Level1State::~Level1State() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> Level1State::update() {
  m_stage.advance(readInput());
  m_stage.compose(m_frame);
  m_videoSystem.updateFrameImage(FRAME, street::StreetStage::FRAME_WIDTH,
                                 street::StreetStage::FRAME_HEIGHT, m_frame);
  m_videoSystem.drawImage(FRAME, 0, 0);

  switch (m_stage.outcome()) {
  case street::StreetStage::Outcome::GameOver:
  case street::StreetStage::Outcome::Quit:
    restartMenuMusic();
    return EngineStateEnum::Menu;
  case street::StreetStage::Outcome::Playing:
  case street::StreetStage::Outcome::LevelFinished:
    break;
  }
  return std::nullopt;
}

const street::StreetStage &Level1State::stage() const { return m_stage; }

street::StreetInput Level1State::readInput() const {
  street::StreetInput input;
  input.joystick = m_controllerSystem.joystick();
  if (const auto key = m_controllerSystem.functionKey()) {
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

void Level1State::restartMenuMusic() {
  m_audioSystem.stopSFX();
  m_audioSystem.loadMusic(MENU_MUSIC_PATH);
  m_audioSystem.playMusic();
  m_audioSystem.setMusicVolume(m_options.music ? MENU_MUSIC_VOLUME : 0);
}

} // namespace openfranko::src::engine::states::level1
