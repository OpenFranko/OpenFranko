#include "PaletteFlasher.h"

#include <stdexcept>
#include <utility>

namespace openfranko::src::engine::effects {

void PaletteFlasher::start(std::size_t color, FlashSteps steps) {
  if (steps.size() > MAX_STEPS) {
    throw std::invalid_argument("A flash list holds at most 16 colours");
  }
  for (const FlashStep &step : steps) {
    if (step.frames < 1) {
      throw std::invalid_argument("A flash colour must stay at least 1 frame");
    }
  }

  m_color = color;
  m_steps = std::move(steps);
  m_next = 0;
  m_countdown = 1;
}

bool PaletteFlasher::tick(AmigaPalette &palette) {
  if (m_steps.empty() || --m_countdown > 0) {
    return false;
  }

  const FlashStep &step = m_steps[m_next];
  m_countdown = step.frames;
  m_next = (m_next + 1) % m_steps.size();

  AmigaColor &entry = palette.at(m_color);
  const bool changed = entry != step.color;
  entry = step.color;
  return changed;
}

bool PaletteFlasher::isFlashing() const { return !m_steps.empty(); }

} // namespace openfranko::src::engine::effects
