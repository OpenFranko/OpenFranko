#include "ProtectionCheckState.h"

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

Cells randomCells() {
  std::random_device seed;
  std::mt19937 random(seed());
  std::uniform_int_distribution<int> coordinate(
      0, effects::CodeCardCheck::CARD_SIZE - 1);

  Cells cells{};
  for (effects::CodeCardCheck::Cell &cell : cells) {
    cell.x = coordinate(random);
    cell.y = coordinate(random);
  }
  return cells;
}

} // namespace

ProtectionCheckState::ProtectionCheckState(
    systems::VideoSystem &videoSystem, systems::AudioSystem &audioSystem,
    systems::ControllerSystem &controllerSystem)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_controllerSystem(controllerSystem),
      m_check(loadCards(), randomCells()) {
  m_videoSystem.createScreen(QUESTION_SCREEN_ID, QUESTION_SCREEN_WIDTH,
                             QUESTION_SCREEN_HEIGHT);
  m_videoSystem.switchScreen(QUESTION_SCREEN_ID);
  showQuestion();
}

ProtectionCheckState::~ProtectionCheckState() {
  m_videoSystem.clearImage(QUESTION);
  m_videoSystem.clearImage(FAILURE);
  m_videoSystem.fillScreen(0, 0, 0);
}

std::optional<EngineStateEnum> ProtectionCheckState::update() {
  if (!m_check.isFinished()) {
    if (const auto letter = m_controllerSystem.typedLetter()) {
      m_check.answer(*letter);
      if (!m_check.isFinished()) {
        showQuestion();
      } else if (!m_check.isPassed()) {
        showFailure();
      }
    }
  }

  const bool failed = m_check.isFinished() && !m_check.isPassed();
  m_videoSystem.drawImage(failed ? FAILURE : QUESTION, 0, 0);
  return std::nullopt;
}

void ProtectionCheckState::showQuestion() {
  m_videoSystem.loadIndexedImage(QUESTION, QUESTION_PATH);
  const effects::CodeCardCheck::Cell cell = m_check.cell();
  m_videoSystem.xorImageRect(QUESTION, CELL_PITCH * cell.x + BOX_OFFSET,
                             CELL_PITCH * cell.y + BOX_OFFSET, BOX_SIZE,
                             BOX_SIZE, ALL_BITPLANES);
}

void ProtectionCheckState::showFailure() {
  m_audioSystem.stopMusic();
  m_videoSystem.createScreen(FAILURE_SCREEN_ID, FAILURE_SCREEN_WIDTH,
                             FAILURE_SCREEN_HEIGHT);
  m_videoSystem.switchScreen(FAILURE_SCREEN_ID);
  m_videoSystem.loadImage(FAILURE, FAILURE_PATH);
}

} // namespace openfranko::src::engine::states::protectionCheck
