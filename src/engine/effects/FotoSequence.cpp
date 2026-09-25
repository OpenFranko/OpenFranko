#include "FotoSequence.h"

#include "AmigaDisplay.h"

#include <utility>

namespace openfranko::src::engine::effects {
namespace {

constexpr AmigaColor WHITE = 0xFFF;
constexpr AmigaColor BLACK = 0x000;

constexpr int WHITE_FRAMES = 5;
constexpr int FOTO_WAIT_PER_SPEED = 15;
constexpr int FOTO_OPEN_VBLS = 2 * SCREEN_OPEN_VBLS;

} // namespace

FotoSequence::FotoSequence(AmigaPalette picturePalette, Timings timings)
    : m_picturePalette(std::move(picturePalette)),
      m_palette(m_picturePalette.size(), WHITE), m_timings(timings) {}

bool FotoSequence::advance() {
  if (isFinished()) {
    return false;
  }

  if (m_frame == fadeInStart()) {
    m_fader.start(m_palette, m_timings.fadeInSpeed, m_picturePalette);
  } else if (m_frame == fadeOutStart()) {
    m_fader.start(m_palette, m_timings.fadeOutSpeed,
                  AmigaPalette(m_palette.size(), BLACK));
  }

  const bool flashed = m_flasher.tick(m_palette);
  const bool faded = m_fader.tick(m_palette);
  ++m_frame;
  return flashed || faded;
}

void FotoSequence::flash(std::size_t color, FlashSteps steps) {
  m_flasher.start(color, std::move(steps));
}

const AmigaPalette &FotoSequence::palette() const { return m_palette; }

bool FotoSequence::isShown() const {
  const int shownFrame = m_frame - 1;
  return shownFrame >= whiteStart() &&
         shownFrame < closeStart() + SCREEN_CLOSE_SHOWN_VBLS;
}

bool FotoSequence::isFinished() const { return m_frame >= totalFrames(); }

int FotoSequence::frame() const { return m_frame; }

int FotoSequence::holdStart() const {
  return fadeInStart() + FOTO_WAIT_PER_SPEED * m_timings.fadeInSpeed +
         SCREEN_CLOSE_VBLS;
}

int FotoSequence::whiteStart() const {
  return FOTO_OPEN_VBLS + (m_timings.replacesScreen ? SCREEN_CLOSE_VBLS : 0);
}

int FotoSequence::fadeInStart() const { return whiteStart() + WHITE_FRAMES; }

int FotoSequence::fadeOutStart() const {
  return holdStart() + m_timings.holdFrames;
}

int FotoSequence::closeStart() const {
  return fadeOutStart() + m_timings.fadeOutFrames;
}

int FotoSequence::totalFrames() const {
  return closeStart() + SCREEN_CLOSE_VBLS;
}

} // namespace openfranko::src::engine::effects
