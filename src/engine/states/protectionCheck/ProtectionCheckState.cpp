#include "ProtectionCheckState.h"

#include "../../street/LoadingMock.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace openfranko::src::engine::states::protectionCheck {
namespace {

using Cells =
    std::array<effects::CodeCardCheck::Cell, effects::CodeCardCheck::CARDS>;
using Tries = std::array<effects::CodeCardCheck::Cell,
                         effects::CodeCardCheck::STAGE_TRIES>;

constexpr auto QUESTION = "codeCardQuestion";
constexpr auto QUESTION_PATH = "assets/03C1.bmp";
constexpr auto FAILURE = "codeCardFailure";
constexpr auto FAILURE_PATH = "assets/03C2.bmp";
constexpr auto CARDS_PATH = "assets/0384/0384_cards.bin";

constexpr int QUESTION_SCREEN_ID = 6;
constexpr int QUESTION_SCREEN_WIDTH = 320;
constexpr int QUESTION_SCREEN_HEIGHT = 200;
constexpr int FAILURE_SCREEN_ID = 0;
constexpr int FAILURE_SCREEN_WIDTH = 320;
constexpr int FAILURE_SCREEN_HEIGHT = 256;
constexpr int FAILURE_DISPLAY_LINE = 50;

constexpr int STAGE_CHECK_FILES = 2;
constexpr int UNPACK_VBLS = 1;
constexpr int SCREEN_CLOSE_VBLS = 2;
constexpr uint8_t HIDDEN_SCREENS_GREY = 0x55;

constexpr int CELL_PITCH = 15;
constexpr int BOX_OFFSET = 11;
constexpr int BOX_SIZE = 13;
constexpr uint8_t ALL_BITPLANES = 31;

std::vector<uint8_t> loadCards() {
  std::ifstream file(CARDS_PATH, std::ios::binary);
  if (!file) {
    throw std::runtime_error(std::string("Missing code cards: ") + CARDS_PATH);
  }
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

template <typename CellArray> CellArray randomCells() {
  std::random_device seed;
  std::mt19937 random(seed());
  std::uniform_int_distribution<int> coordinate(
      0, effects::CodeCardCheck::CARD_SIZE - 1);

  CellArray cells{};
  for (effects::CodeCardCheck::Cell &cell : cells) {
    cell.x = coordinate(random);
    cell.y = coordinate(random);
  }
  return cells;
}

effects::CodeCardCheck makeCheck(ProtectionCheckState::Check check) {
  if (check == ProtectionCheckState::Check::Stage3) {
    return effects::CodeCardCheck::stageCheck(loadCards(),
                                              randomCells<Tries>());
  }
  return effects::CodeCardCheck(loadCards(), randomCells<Cells>());
}

} // namespace

ProtectionCheckState::ProtectionCheckState(
    systems::VideoSystem &videoSystem, systems::AudioSystem &audioSystem,
    systems::ControllerSystem &controllerSystem, Check check)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_controllerSystem(controllerSystem), m_kind(check),
      m_check(makeCheck(check)),
      m_loadingFrames(check == Check::Stage3
                          ? STAGE_CHECK_FILES * street::LoadingMock::FILE_FRAMES
                          : 0),
      m_resumeFrame(UNPACK_VBLS) {
  m_videoSystem.createScreen(QUESTION_SCREEN_ID, QUESTION_SCREEN_WIDTH,
                             QUESTION_SCREEN_HEIGHT);
  m_videoSystem.switchScreen(QUESTION_SCREEN_ID);
}

ProtectionCheckState::~ProtectionCheckState() {
  m_videoSystem.clearImage(QUESTION);
  m_videoSystem.clearImage(FAILURE);
  m_videoSystem.fillScreen(0, 0, 0);
}

std::optional<EngineStateEnum> ProtectionCheckState::update() {
  if (m_loadingFrames > 0) {
    --m_loadingFrames;
    m_videoSystem.fillScreen(HIDDEN_SCREENS_GREY, HIDDEN_SCREENS_GREY,
                             HIDDEN_SCREENS_GREY);
    return std::nullopt;
  }
  if (const auto letter = m_controllerSystem.typedLetter()) {
    m_typed.push_back(*letter);
  }
  const std::optional<EngineStateEnum> next = runCheck();
  ++m_frame;
  if (next) {
    return next;
  }
  draw();
  return std::nullopt;
}

std::optional<EngineStateEnum> ProtectionCheckState::runCheck() {
  while (m_frame >= m_resumeFrame) {
    switch (m_step) {
    case Step::Unpack:
      showQuestion();
      m_questionShown = true;
      m_step = Step::Ask;
      break;
    case Step::Ask:
      if (!takeAnswer()) {
        return std::nullopt;
      }
      m_questionShown = false;
      m_resumeFrame = m_frame + SCREEN_CLOSE_VBLS;
      m_step = Step::Closed;
      break;
    case Step::Closed:
      if (!m_check.isFinished()) {
        m_resumeFrame = m_frame + UNPACK_VBLS;
        m_step = Step::Unpack;
      } else if (m_check.isPassed()) {
        return m_kind == Check::Stage3 ? EngineStateEnum::Level3
                                       : EngineStateEnum::Menu;
      } else {
        showFailure();
        m_resumeFrame = m_frame + UNPACK_VBLS;
        m_step = Step::FailureUnpacked;
      }
      break;
    case Step::FailureUnpacked:
      m_audioSystem.stopMusic();
      m_failureShown = true;
      m_step = Step::Hang;
      break;
    case Step::Hang:
      return std::nullopt;
    }
  }
  return std::nullopt;
}

bool ProtectionCheckState::takeAnswer() {
  while (!m_typed.empty()) {
    const char letter = m_typed.front();
    m_typed.pop_front();
    if (letter >= effects::CodeCardCheck::FIRST_ANSWER &&
        letter <= effects::CodeCardCheck::LAST_ANSWER) {
      m_check.answer(letter);
      return true;
    }
  }
  return false;
}

void ProtectionCheckState::draw() {
  if (m_failureShown) {
    m_videoSystem.drawImage(FAILURE, 0, -m_failureTop);
  } else if (m_questionShown) {
    m_videoSystem.drawImage(QUESTION, 0, 0);
  } else if (m_kind == Check::Stage3) {
    m_videoSystem.fillScreen(HIDDEN_SCREENS_GREY, HIDDEN_SCREENS_GREY,
                             HIDDEN_SCREENS_GREY);
  } else {
    m_videoSystem.fillScreen(0, 0, 0);
  }
}

const effects::CodeCardCheck &ProtectionCheckState::check() const {
  return m_check;
}

void ProtectionCheckState::showQuestion() {
  m_videoSystem.loadIndexedImage(QUESTION, QUESTION_PATH);
  const effects::CodeCardCheck::Cell cell = m_check.cell();
  m_videoSystem.xorImageRect(QUESTION, CELL_PITCH * cell.x + BOX_OFFSET,
                             CELL_PITCH * cell.y + BOX_OFFSET, BOX_SIZE,
                             BOX_SIZE, ALL_BITPLANES);
}

void ProtectionCheckState::showFailure() {
  const bool ntsc = m_videoSystem.isNtsc();
  const effects::VisibleRows rows =
      effects::visibleRows(effects::pictureLine(FAILURE_DISPLAY_LINE, ntsc),
                           FAILURE_SCREEN_HEIGHT, ntsc);
  m_failureTop = rows.first;
  m_videoSystem.createScreen(FAILURE_SCREEN_ID, FAILURE_SCREEN_WIDTH,
                             rows.count);
  m_videoSystem.switchScreen(FAILURE_SCREEN_ID);
  m_videoSystem.loadImage(FAILURE, FAILURE_PATH);
}

} // namespace openfranko::src::engine::states::protectionCheck
