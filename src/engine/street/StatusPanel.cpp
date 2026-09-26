#include "StatusPanel.h"

#include "../amal/Actors.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>

namespace openfranko::src::engine::street {
namespace {

constexpr uint8_t EMPTY_BAR_COLOR = 6;
constexpr uint8_t FULL_BAR_COLOR = 2;
constexpr int BAR_LEFT = 111;
constexpr int BAR_RIGHT = 176;
constexpr int BAR_TOP = 13;
constexpr int BAR_BOTTOM = 15;
constexpr int GLYPH_TOP = 32;
constexpr int DIGIT_BOTTOM = 39;
constexpr int LIVES_DIGIT_X = 28;
constexpr int VERSION12_LIVES_DIGIT_X = 31;

} // namespace

StatusPanel::StatusPanel(Picture loadingStrip, Picture artwork,
                         GameVersion version)
    : m_loadingStrip(std::move(loadingStrip)), m_artwork(std::move(artwork)),
      m_livesDigitX(version == GameVersion::V12 ? VERSION12_LIVES_DIGIT_X
                                                : LIVES_DIGIT_X),
      m_surface(WIDTH, HEIGHT) {}

void StatusPanel::showLoading() { m_surface.unpack(m_loadingStrip, 0, 0); }

void StatusPanel::showWaiting() {
  showLoading();
  m_surface.copy(m_surface, 203, GLYPH_TOP, 303, 47, 101, 10);
}

void StatusPanel::score(const Stats &stats) {
  m_surface.unpack(m_artwork, 0, 0);
  loseEnergy(stats.energy);
  m_surface.copy(m_surface, 8 * stats.stage, GLYPH_TOP, 7 + 8 * stats.stage,
                 DIGIT_BOTTOM, 48, 9);
  drawKills(stats.kills);
  const int icons = std::min(stats.lives, 3);
  m_surface.copy(m_surface, 80, GLYPH_TOP, 90 + 12 * (icons - 1), 41, 6, 8);
  if (stats.lives > 3) {
    const int digit = std::min(stats.lives, 9);
    m_surface.copy(m_surface, 8 * digit, GLYPH_TOP, 7 + 8 * digit, DIGIT_BOTTOM,
                   m_livesDigitX, 19);
  }
}

void StatusPanel::drawKills(int kills) {
  const std::string digits = std::to_string(std::abs(kills));
  const int length = static_cast<int>(digits.size());
  const int offset = -4 * amal::actors::amosBool(length == 2) -
                     8 * amal::actors::amosBool(length == 1);
  for (int i = 0; i < length; ++i) {
    const int digit = digits[static_cast<std::size_t>(i)] - '0';
    m_surface.copy(m_surface, 8 * digit, GLYPH_TOP, 8 + 8 * digit, DIGIT_BOTTOM,
                   70 + offset + 8 * i, 9);
  }
}

void StatusPanel::loseEnergy(int energy) {
  m_surface.clear(EMPTY_BAR_COLOR, BAR_LEFT + energy, BAR_TOP, BAR_RIGHT,
                  BAR_BOTTOM);
}

void StatusPanel::gainEnergy(int energy) {
  m_surface.clear(FULL_BAR_COLOR, BAR_LEFT, BAR_TOP, BAR_LEFT + energy,
                  BAR_BOTTOM);
}

const IndexedSurface &StatusPanel::surface() const { return m_surface; }

} // namespace openfranko::src::engine::street
