#include "FotoScreen.h"

#include <cstddef>

namespace openfranko::src::engine::states::shared {
namespace {

constexpr int SCREEN_WIDTH = 368;
constexpr int SCREEN_HEIGHT = 290;
constexpr std::size_t SCREEN_COLORS = 32;

effects::color::AmigaPalette
screenPalette(const systems::graphics::IndexedBitmap &picture) {
  effects::color::AmigaPalette palette = picture.palette;
  palette.resize(SCREEN_COLORS);
  return palette;
}

} // namespace

FotoScreen::FotoScreen(systems::graphics::VideoSystem &videoSystem,
                       const std::string &picturePath, int displayLine,
                       const effects::sequences::FotoSequence::Timings &timings)
    : m_videoSystem(videoSystem),
      m_rows(visibleRows(displayLine, SCREEN_HEIGHT, videoSystem.isNtsc())),
      m_picture(systems::graphics::loadIndexedBitmap(picturePath)),
      m_screen(SCREEN_WIDTH, m_rows.count),
      m_sequence(screenPalette(m_picture), timings) {}

effects::sequences::FotoSequence &FotoScreen::sequence() { return m_sequence; }

void FotoScreen::advance() {
  m_sequence.advance();
  if (m_sequence.isShown()) {
    m_screen.setPalette(m_sequence.palette());
    m_screen.draw(m_picture, 0, -m_rows.first);
  } else {
    m_screen.fill(effects::color::BLACK);
  }
  m_videoSystem.show(m_screen.output());
}

} // namespace openfranko::src::engine::states::shared
