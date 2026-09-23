#include "FotoSequence.h"

#include <utility>

namespace openfranko::src::engine::effects {
namespace {

constexpr AmigaColor WHITE = 0xFFF;
constexpr AmigaColor BLACK = 0x000;

constexpr int WHITE_FRAMES = 5;
constexpr int FOTO_WAIT_PER_SPEED = 15;

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

  const bool changed = m_fader.tick(m_palette);
  ++m_frame;
  return changed;
}

const AmigaPalette &FotoSequence::palette() const { return m_palette; }

bool FotoSequence::isFinished() const { return m_frame >= totalFrames(); }

int FotoSequence::fadeInStart() const { return WHITE_FRAMES; }

int FotoSequence::fadeOutStart() const {
  return fadeInStart() + FOTO_WAIT_PER_SPEED * m_timings.fadeInSpeed +
         m_timings.holdFrames;
}

int FotoSequence::totalFrames() const {
  return fadeOutStart() + m_timings.fadeOutFrames;
}

} // namespace openfranko::src::engine::effects
