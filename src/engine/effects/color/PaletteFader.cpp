#include "PaletteFader.h"

#include <iterator>
#include <stdexcept>

namespace openfranko::src::engine::effects {
namespace {

constexpr AmigaColor COLOR_MASK = 0x0FFF;
constexpr AmigaColor NEGATIVE_BIT = 0x8000;

AmigaColor stepToward(AmigaColor current, AmigaColor target) {
  AmigaColor result = 0;
  for (int shift : {8, 4, 0}) {
    int value = (current >> shift) & 0xF;
    const int goal = (target >> shift) & 0xF;
    if (value < goal) {
      ++value;
    } else if (value > goal) {
      --value;
    }
    result |= static_cast<AmigaColor>(value << shift);
  }
  return result;
}

} // namespace

void PaletteFader::start(const AmigaPalette &palette, int speed,
                         const AmigaPalette &target) {
  if (speed < 1) {
    throw std::invalid_argument("Fade speed must be at least 1");
  }

  m_speed = speed;
  m_countdown = 1;
  m_current = palette;
  m_target.assign(palette.size(), KEEP);
  m_fading.clear();

  for (std::size_t i = 0; i < palette.size() && i < target.size(); ++i) {
    if (target[i] & NEGATIVE_BIT) {
      continue;
    }
    m_current[i] &= COLOR_MASK;
    m_target[i] = target[i] & COLOR_MASK;
    if (m_current[i] != m_target[i]) {
      m_fading.push_back(i);
    }
  }
}

bool PaletteFader::tick(AmigaPalette &palette) {
  if (m_fading.empty() || --m_countdown > 0) {
    return false;
  }
  m_countdown = m_speed;

  for (auto it = m_fading.begin(); it != m_fading.end();) {
    const std::size_t index = *it;
    m_current[index] = stepToward(m_current[index], m_target[index]);
    if (index < palette.size()) {
      palette[index] = m_current[index];
    }
    it = (m_current[index] == m_target[index]) ? m_fading.erase(it)
                                               : std::next(it);
  }
  return true;
}

bool PaletteFader::isFading() const { return !m_fading.empty(); }

} // namespace openfranko::src::engine::effects
