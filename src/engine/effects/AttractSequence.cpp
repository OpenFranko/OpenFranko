#include "AttractSequence.h"

#include "PaletteFader.h"

#include <array>
#include <cstddef>
#include <utility>

namespace openfranko::src::engine::effects {
namespace {

constexpr int TITLE_INPUT_FROM = 10;
constexpr int TITLE_HOLD = 200;

constexpr int DIM_STEPS = 4;
constexpr int DIM_EVERY = 4;
constexpr int ROWS_FROM = DIM_STEPS * DIM_EVERY;
constexpr int ROW_EVERY = 10;
constexpr int WAIT_AFTER_ROWS = 10;
constexpr int HISCORE_INPUT_FROM =
    ROWS_FROM + AttractSequence::HISCORE_ROWS * ROW_EVERY + WAIT_AFTER_ROWS;
constexpr int HISCORE_HOLD = 400;

struct Relit {
  std::size_t index;
  AmigaColor color;
};

constexpr std::array<Relit, 3> RELIT = {{
    {29, 0x769},
    {30, 0xB95},
    {31, 0xFC0},
}};

bool isOver(int frame, int inputFrom, int hold, bool joystickTouched) {
  return frame >= inputFrom && (joystickTouched || frame > inputFrom + hold);
}

} // namespace

AttractSequence::AttractSequence(Kind kind, AmigaPalette picturePalette)
    : m_kind(kind), m_palette(std::move(picturePalette)) {}

void AttractSequence::advance(bool joystickTouched) {
  if (m_finished) {
    return;
  }
  m_waiting =
      m_frame < (m_kind == Kind::Title ? TITLE_INPUT_FROM : HISCORE_INPUT_FROM);
  if (m_kind == Kind::Title) {
    advanceTitle(joystickTouched);
  } else {
    advanceHiscores(joystickTouched);
  }
  m_showing = !m_finished && (m_kind == Kind::Title || m_frame > 0);
  ++m_frame;
}

AttractSequence::Kind AttractSequence::kind() const { return m_kind; }

bool AttractSequence::isShowing() const { return m_showing; }

const AmigaPalette &AttractSequence::palette() const { return m_palette; }

int AttractSequence::rowsShown() const { return m_rows; }

bool AttractSequence::isWaiting() const { return m_waiting; }

bool AttractSequence::isFinished() const { return m_finished; }

void AttractSequence::advanceTitle(bool joystickTouched) {
  m_finished = isOver(m_frame, TITLE_INPUT_FROM, TITLE_HOLD, joystickTouched);
}

void AttractSequence::advanceHiscores(bool joystickTouched) {
  if (m_frame < ROWS_FROM && m_frame % DIM_EVERY == 0) {
    PaletteFader dim;
    dim.start(m_palette, 1, AmigaPalette(m_palette.size(), 0));
    dim.tick(m_palette);
  }
  if (m_frame == ROWS_FROM) {
    for (const Relit &relit : RELIT) {
      if (relit.index < m_palette.size()) {
        m_palette[relit.index] = relit.color;
      }
    }
  }
  if (m_frame >= ROWS_FROM && m_rows < HISCORE_ROWS &&
      (m_frame - ROWS_FROM) % ROW_EVERY == 0) {
    ++m_rows;
  }
  m_finished =
      isOver(m_frame, HISCORE_INPUT_FROM, HISCORE_HOLD, joystickTouched);
}

} // namespace openfranko::src::engine::effects
