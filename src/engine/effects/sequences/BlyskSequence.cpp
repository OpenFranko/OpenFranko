#include "BlyskSequence.h"

namespace openfranko::src::engine::effects {
namespace {

constexpr AmigaColor BLACK = 0x000;
constexpr AmigaColor INK = 0xFFF;
constexpr AmigaColor SHADE = 0xAAA;

const AmigaPalette LIT = {BLACK, INK, SHADE, PaletteFader::KEEP};
const AmigaPalette DARK = {BLACK, BLACK, BLACK, PaletteFader::KEEP};

} // namespace

BlyskSequence::BlyskSequence(int firstPage, int endPage)
    : m_page(firstPage), m_endPage(endPage) {
  if (m_page >= m_endPage) {
    m_finished = true;
  }
}

void BlyskSequence::advance(bool fireLatched) {
  if (m_finished) {
    return;
  }
  m_fader.tick(m_palette);
  if (m_time == 0) {
    startPage();
  } else if (m_time == LIT_FRAMES) {
    m_fader.start(m_palette, FADE_SPEED, DARK);
  } else if (m_time == PAGE_FRAMES) {
    m_pasted = false;
    if (fireLatched) {
      m_finished = true;
      m_skipped = true;
      return;
    }
    if (++m_page == m_endPage) {
      m_finished = true;
      return;
    }
    m_time = 0;
    startPage();
  }
  ++m_time;
}

std::optional<int> BlyskSequence::page() const {
  return m_pasted ? std::optional<int>(m_page) : std::nullopt;
}

const AmigaPalette &BlyskSequence::palette() const { return m_palette; }

bool BlyskSequence::isFinished() const { return m_finished; }

bool BlyskSequence::isSkipped() const { return m_skipped; }

void BlyskSequence::startPage() {
  m_pasted = true;
  m_fader.start(m_palette, FADE_SPEED, LIT);
}

} // namespace openfranko::src::engine::effects
