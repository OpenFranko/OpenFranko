#include "Engine.h"

#include "../systems/graphics/PixelOps.h"

#include "assets/Assets.h"
#include "assets/GameFiles.h"
#include "assets/RequiredFiles.h"
#include "assets/YieldingFiles.h"
#include "states/adverts/AdvertsState.h"
#include "states/characterSelection/CharacterSelectionState.h"
#include "states/continueSelect/ContinueState.h"
#include "states/ending/EndingState.h"
#include "states/gameOver/GameOverState.h"
#include "states/highScore/HighScoreState.h"
#include "states/kneeAnimation/KneeAnimationState.h"
#include "states/level1/Level1BossState.h"
#include "states/level1/Level1CarState.h"
#include "states/level1/Level1State.h"
#include "states/level2/Level2BossState.h"
#include "states/level2/Level2CarState.h"
#include "states/level2/Level2State.h"
#include "states/level3/Level3BossState.h"
#include "states/level3/Level3State.h"
#include "states/menu/MenuState.h"
#include "states/mirage/MirageState.h"
#include "states/presents/PresentsState.h"
#include "states/protectionCheck/ProtectionCheckState.h"
#include "states/spiderLogo/SpiderLogoState.h"
#include "states/titleAndStory/TitleAndStoryState.h"
#include "states/worldSoftware/WorldSoftwareState.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace openfranko::src::engine {
namespace {

constexpr std::size_t LISTED_MISSING_FILES = 8;

systems::input::KeyMode keyMode(states::EngineStateId state) {
  switch (state) {
  case states::EngineStateId::Level1:
  case states::EngineStateId::Level1Boss:
  case states::EngineStateId::Level1Car:
  case states::EngineStateId::Level2:
  case states::EngineStateId::Level2Boss:
  case states::EngineStateId::Level2Car:
  case states::EngineStateId::Level3:
  case states::EngineStateId::Level3Boss:
    return systems::input::KeyMode::Game;
  case states::EngineStateId::HighScore:
    return systems::input::KeyMode::NameEntry;
  case states::EngineStateId::Mirage:
  case states::EngineStateId::SpiderLogo:
  case states::EngineStateId::Adverts:
  case states::EngineStateId::Presents:
  case states::EngineStateId::WorldSoftware:
  case states::EngineStateId::KneeAnimation:
  case states::EngineStateId::TitleAndStory:
  case states::EngineStateId::ProtectionCheck:
  case states::EngineStateId::Menu:
  case states::EngineStateId::CharacterSelectionSequence:
  case states::EngineStateId::StageProtectionCheck:
  case states::EngineStateId::Ending:
  case states::EngineStateId::GameOver:
  case states::EngineStateId::Continue:
    break;
  }
  return systems::input::KeyMode::FrontEnd;
}

void saveHighScores(const street::core::HighScoreTable &table) {
  street::core::writeHighScoreFile(table,
                                   street::core::HighScoreTable::FILE_NAME);
}

std::string versionName(GameVersion version) {
  return version == GameVersion::V12 ? "1.2" : "1.0";
}

void requireGameData(const assets::Files &files) {
  if (!files.exists(assets::DIRECTORY)) {
    throw std::runtime_error(
        "No game data found. Run the game from the directory that holds\n"
        "assets.tar or the assets directory made by frankoExtract.");
  }
  const GameVersion version = assets::detectVersion(files);
  const std::vector<std::string> missing = assets::missingFiles(files, version);
  if (missing.empty()) {
    return;
  }
  std::string message = "The Franko " + versionName(version) +
                        " game data is incomplete, these files are missing:";
  for (std::size_t i = 0; i < missing.size() && i < LISTED_MISSING_FILES; ++i) {
    message += "\n  " + missing[i];
  }
  if (missing.size() > LISTED_MISSING_FILES) {
    message += "\n  and " +
               std::to_string(missing.size() - LISTED_MISSING_FILES) + " more";
  }
  throw std::runtime_error(message +
                           "\nExtract the game data again with frankoExtract.");
}

std::unique_ptr<assets::Files> openCheckedFiles() {
  std::unique_ptr<assets::Files> files = assets::openGameFiles();
  requireGameData(*files);
  return files;
}

} // namespace

Engine::Engine()
    : Engine(states::EngineStateId::Mirage, street::session::GameSession{}) {}

Engine::Engine(states::EngineStateId firstState,
               street::session::GameSession startingSession)
    : m_audioSystem(
          [this](const std::string &path) { return m_files->read(path); }),
      m_files(std::make_unique<assets::PrefetchingFiles>(
          std::make_unique<assets::YieldingFiles>(
              openCheckedFiles(), [this] { m_audioSystem.update(); }))),
      m_menuPrefetch(*m_files, m_audioSystem),
      m_session(std::move(startingSession)), m_running(true) {
  m_options.ntsc = m_videoSystem.isNtsc();
  m_session.version = assets::detectVersion(*m_files);
  m_session.highScores =
      street::core::readHighScoreFile(street::core::HighScoreTable::FILE_NAME)
          .value_or(street::core::HighScoreTable(m_session.version));
  switchState(firstState);
}

Engine::~Engine() { m_currentState.reset(); }

bool Engine::isRunning() {
  if (!m_platform.pollEvents(m_controllerSystem)) {
    m_running = false;
  }
  return m_running;
}

void Engine::updateState() {
  if (!m_currentState) {
    return;
  }
  std::optional<states::EngineStateId> nextState = m_currentState->update();
  while (nextState) {
    systems::graphics::pixels::finish();
    switchState(*nextState);
    nextState = m_currentState->update();
  }
}

states::EngineStateId Engine::versionState(states::EngineStateId state) const {
  if (m_session.version != GameVersion::V12) {
    return state;
  }
  switch (state) {
  case states::EngineStateId::Mirage:
    return states::EngineStateId::SpiderLogo;
  case states::EngineStateId::ProtectionCheck:
    return states::EngineStateId::HighScore;
  case states::EngineStateId::StageProtectionCheck:
    return states::EngineStateId::Level3;
  default:
    return state;
  }
}

void Engine::switchState(states::EngineStateId nextState) {
  nextState = versionState(nextState);
#ifdef SKIP_COPY_PROTECTION
  if (nextState == states::EngineStateId::ProtectionCheck) {
    nextState = states::EngineStateId::HighScore;
  } else if (nextState == states::EngineStateId::StageProtectionCheck) {
    nextState = states::EngineStateId::Level3;
  }
#endif
  m_videoSystem.clear();
  m_currentState.reset();
  m_streetHost.reset();
  m_booting = nextState == states::EngineStateId::Mirage;
  m_controllerSystem.setKeyMode(keyMode(nextState));
  if (nextState != states::EngineStateId::Menu) {
    m_session.keyboard.permit();
  }

  switch (nextState) {
  case states::EngineStateId::Mirage:
    m_currentState =
        std::make_unique<states::mirage::MirageState>(m_videoSystem, *m_files);
    break;
  case states::EngineStateId::SpiderLogo:
    m_currentState = std::make_unique<states::spiderLogo::SpiderLogoState>(
        m_videoSystem, m_audioSystem, *m_files);
    break;
  case states::EngineStateId::Adverts:
    m_currentState = std::make_unique<states::adverts::AdvertsState>(
        m_videoSystem, m_controllerSystem, *m_files);
    break;
  case states::EngineStateId::Presents:
    m_currentState = std::make_unique<states::presents::PresentsState>(
        m_videoSystem, m_audioSystem, m_controllerSystem, *m_files);
    break;
  case states::EngineStateId::WorldSoftware:
    m_currentState =
        std::make_unique<states::worldSoftware::WorldSoftwareState>(
            m_videoSystem, m_audioSystem, *m_files);
    break;
  case states::EngineStateId::KneeAnimation:
    m_currentState =
        std::make_unique<states::kneeAnimation::KneeAnimationState>(
            m_videoSystem, m_audioSystem, m_controllerSystem, *m_files,
            m_session.version);
    break;
  case states::EngineStateId::TitleAndStory:
    m_currentState =
        std::make_unique<states::titleAndStory::TitleAndStoryState>(
            m_videoSystem, m_audioSystem, m_controllerSystem, *m_files,
            m_session.version);
    break;
  case states::EngineStateId::ProtectionCheck:
    m_currentState =
        std::make_unique<states::protectionCheck::ProtectionCheckState>(
            m_videoSystem, m_audioSystem, *m_files, m_session.keyboard);
    break;
  case states::EngineStateId::Menu:
    m_currentState = std::make_unique<states::menu::MenuState>(
        m_videoSystem, m_audioSystem, m_controllerSystem, *m_files, m_options,
        m_session);
    break;
  case states::EngineStateId::CharacterSelectionSequence:
    m_currentState =
        std::make_unique<states::characterSelection::CharacterSelectionState>(
            m_videoSystem, m_audioSystem, m_controllerSystem, *m_files,
            m_options, m_session);
    break;
  case states::EngineStateId::Level1:
    m_currentState = std::make_unique<states::level1::Level1State>(
        m_videoSystem, makeStreetHost(), m_controllerSystem, m_options,
        m_session);
    break;
  case states::EngineStateId::Level1Boss:
    m_currentState = std::make_unique<states::level1::Level1BossState>(
        m_videoSystem, makeStreetHost(), m_controllerSystem, m_options,
        m_session);
    break;
  case states::EngineStateId::Level1Car:
    m_currentState = std::make_unique<states::level1::Level1CarState>(
        m_videoSystem, makeStreetHost(), m_controllerSystem, m_options,
        m_session);
    break;
  case states::EngineStateId::Level2:
    m_currentState = std::make_unique<states::level2::Level2State>(
        m_videoSystem, makeStreetHost(), m_controllerSystem, m_options,
        m_session);
    break;
  case states::EngineStateId::Level2Boss:
    m_currentState = std::make_unique<states::level2::Level2BossState>(
        m_videoSystem, makeStreetHost(), m_controllerSystem, m_options,
        m_session);
    break;
  case states::EngineStateId::Level2Car:
    m_currentState = std::make_unique<states::level2::Level2CarState>(
        m_videoSystem, makeStreetHost(), m_controllerSystem, m_options,
        m_session);
    break;
  case states::EngineStateId::StageProtectionCheck:
    m_currentState =
        std::make_unique<states::protectionCheck::ProtectionCheckState>(
            m_videoSystem, m_audioSystem, *m_files, m_session.keyboard,
            states::protectionCheck::ProtectionCheckState::Check::Stage3);
    break;
  case states::EngineStateId::Level3:
    m_currentState = std::make_unique<states::level3::Level3State>(
        m_videoSystem, makeStreetHost(), m_controllerSystem, m_options,
        m_session);
    break;
  case states::EngineStateId::Level3Boss:
    m_currentState = std::make_unique<states::level3::Level3BossState>(
        m_videoSystem, makeStreetHost(), m_controllerSystem, m_options,
        m_session);
    break;
  case states::EngineStateId::Ending:
    m_currentState = std::make_unique<states::ending::EndingState>(
        m_videoSystem, makeStreetHost(), m_controllerSystem, m_options,
        m_session);
    break;
  case states::EngineStateId::GameOver:
    m_currentState = std::make_unique<states::gameOver::GameOverState>(
        m_videoSystem, makeStreetHost(), m_controllerSystem, m_options,
        m_session);
    break;
  case states::EngineStateId::HighScore:
    m_currentState = std::make_unique<states::highScore::HighScoreState>(
        m_videoSystem, makeStreetHost(), m_options, m_session, saveHighScores);
    break;
  case states::EngineStateId::Continue:
    m_currentState = std::make_unique<states::continueSelect::ContinueState>(
        m_videoSystem, makeStreetHost(), m_controllerSystem, m_options,
        m_session);
    break;
  }
  if (nextState == states::EngineStateId::HighScore) {
    m_menuPrefetch.start(m_session.version);
  } else if (nextState == states::EngineStateId::Menu) {
    m_menuPrefetch.pause();
  } else if (nextState != states::EngineStateId::Continue) {
    m_menuPrefetch.stop();
  }
}

states::shared::EngineStreetHost &Engine::makeStreetHost() {
  m_streetHost = std::make_unique<states::shared::EngineStreetHost>(
      m_audioSystem, *m_files, m_session.version, m_random,
      [this] { m_audioSystem.update(); });
  return *m_streetHost;
}

void Engine::update() {
  m_controllerSystem.update();
  if (m_booting && m_controllerSystem.isDeleteHeld()) {
    m_session.highScores = street::core::HighScoreTable(m_session.version);
  }
  for (const char key : m_controllerSystem.typedKeys()) {
    m_session.keyboard.press(key);
  }
  m_audioSystem.update();
  updateState();
  m_menuPrefetch.step();
  m_controllerSystem.setEnteringText(m_currentState &&
                                     m_currentState->isEnteringText());
  m_videoSystem.sync();
}

void Engine::run() {
  while (isRunning()) {
    update();
  }
}

} // namespace openfranko::src::engine
