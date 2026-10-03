#include "BlyskSequence.h"

namespace openfranko::src::engine::effects::sequences {
namespace {

const color::AmigaPalette LIT = {color::BLACK, BlyskSequence::INK,
                                 BlyskSequence::SHADE,
                                 color::PaletteFader::KEEP};
const color::AmigaPalette DARK = {color::BLACK, color::BLACK, color::BLACK,
                                  color::PaletteFader::KEEP};

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
  m_fader.advance(m_palette);
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

std::optional<int> BlyskSequence::nextPage() const {
  return !m_finished && m_page + 1 < m_endPage ? std::optional<int>(m_page + 1)
                                               : std::nullopt;
}

bool BlyskSequence::isSteady() const { return m_pasted && !m_fader.isFading(); }

const color::AmigaPalette &BlyskSequence::palette() const { return m_palette; }

bool BlyskSequence::isFinished() const { return m_finished; }

bool BlyskSequence::isSkipped() const { return m_skipped; }

void BlyskSequence::startPage() {
  m_pasted = true;
  m_fader.start(m_palette, FADE_SPEED, LIT);
}

} // namespace openfranko::src::engine::effects::sequences
