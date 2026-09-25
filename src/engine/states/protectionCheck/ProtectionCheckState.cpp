#include "ProtectionCheckState.h"

#include "../../street/LoadingMock.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
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

constexpr auto QUESTION_PATH = "assets/03C1.bmp";
constexpr auto FAILURE_PATH = "assets/03C2.bmp";
constexpr auto CARDS_PATH = "assets/0384/0384_cards.bin";

constexpr int QUESTION_SCREEN_WIDTH = 320;
constexpr int QUESTION_SCREEN_HEIGHT = 200;
constexpr int FAILURE_SCREEN_WIDTH = 320;
constexpr int FAILURE_SCREEN_HEIGHT = 256;
constexpr int FAILURE_DISPLAY_LINE = 50;

constexpr int STAGE_CHECK_FILES = 2;
constexpr effects::AmigaColor STAGE_BORDER = 0x555;
constexpr effects::AmigaColor BLACK = 0x000;

constexpr int CELL_PITCH = 15;
constexpr int BOX_OFFSET = 11;
constexpr int BOX_SIZE = 13;
constexpr uint8_t BOX_INK = 15;
const effects::FlashSteps BOX_FLASH = {{0xFFF, 5}, {0x000, 5}};

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

void xorRect(systems::IndexedBitmap &image, int x, int y, int width, int height,
             uint8_t mask) {
  const int left = std::max(x, 0);
  const int top = std::max(y, 0);
  const int right = std::min(x + width, image.width);
  const int bottom = std::min(y + height, image.height);
  for (int row = top; row < bottom; ++row) {
    for (int column = left; column < right; ++column) {
      image.pixels[static_cast<std::size_t>(row * image.width + column)] ^=
          mask;
    }
  }
}

} // namespace

ProtectionCheckState::ProtectionCheckState(systems::VideoSystem &videoSystem,
                                           systems::AudioSystem &audioSystem,
                                           effects::InkeyBuffer &keyboard,
                                           Check check)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_keyboard(keyboard), m_kind(check), m_check(makeCheck(check)),
      m_loadingFrames(check == Check::Stage3
                          ? STAGE_CHECK_FILES * street::LoadingMock::FILE_FRAMES
                          : 0),
      m_resumeFrame(effects::SCREEN_OPEN_VBLS),
      m_screen(QUESTION_SCREEN_WIDTH, QUESTION_SCREEN_HEIGHT),
      m_border(check == Check::Stage3 ? STAGE_BORDER : BLACK) {}

std::optional<EngineStateEnum> ProtectionCheckState::update() {
  if (m_loadingFrames > 0) {
    --m_loadingFrames;
    m_screen.fill(m_border);
    show();
    return std::nullopt;
  }
  const std::optional<EngineStateEnum> next = runCheck();
  if (m_questionShown) {
    m_flasher.tick(m_questionPalette);
  }
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
      m_resumeFrame = m_frame + effects::SCREEN_CLOSE_SHOWN_VBLS;
      m_step = Step::Hidden;
      break;
    case Step::Hidden:
      m_questionShown = false;
      m_resumeFrame = m_frame + effects::SCREEN_CLOSE_HIDDEN_VBLS;
      m_step = Step::Closed;
      break;
    case Step::Closed:
      if (!m_check.isFinished()) {
        m_resumeFrame = m_frame + effects::SCREEN_OPEN_VBLS;
        m_step = Step::Unpack;
      } else if (m_check.isPassed()) {
        return m_kind == Check::Stage3 ? EngineStateEnum::Level3
                                       : EngineStateEnum::HighScore;
      } else {
        showFailure();
        m_resumeFrame =
            m_frame + (m_kind == Check::Stage3 ? effects::SCREEN_REOPEN_VBLS
                                               : effects::SCREEN_OPEN_VBLS);
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
  while (const std::optional<char> key = m_keyboard.inkey()) {
    const char letter =
        static_cast<char>(std::toupper(static_cast<unsigned char>(*key)));
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
    m_screen.setPalette(m_failure.palette);
    m_screen.draw(m_failure, 0, -m_failureTop);
  } else if (m_questionShown) {
    m_screen.setPalette(m_questionPalette);
    m_screen.draw(m_question, 0, 0);
  } else {
    m_screen.fill(m_border);
  }
  show();
}

void ProtectionCheckState::show() { m_videoSystem.show(m_screen.output()); }

const effects::CodeCardCheck &ProtectionCheckState::check() const {
  return m_check;
}

void ProtectionCheckState::showQuestion() {
  m_question = systems::loadIndexedBitmap(QUESTION_PATH);
  m_questionPalette = m_question.palette;
  m_border = m_questionPalette[0];
  m_flasher.start(BOX_INK, BOX_FLASH);
  const effects::CodeCardCheck::Cell cell = m_check.cell();
  xorRect(m_question, CELL_PITCH * cell.x + BOX_OFFSET,
          CELL_PITCH * cell.y + BOX_OFFSET, BOX_SIZE, BOX_SIZE, BOX_INK);
}

void ProtectionCheckState::showFailure() {
  const bool ntsc = m_videoSystem.isNtsc();
  const effects::VisibleRows rows =
      effects::visibleRows(effects::pictureLine(FAILURE_DISPLAY_LINE, ntsc),
                           FAILURE_SCREEN_HEIGHT, ntsc);
  m_failureTop = rows.first;
  m_screen = systems::Canvas(FAILURE_SCREEN_WIDTH, rows.count);
  m_failure = systems::loadIndexedBitmap(FAILURE_PATH);
}

} // namespace openfranko::src::engine::states::protectionCheck
