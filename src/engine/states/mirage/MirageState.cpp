#include "MirageState.h"

#include <cstddef>

namespace openfranko::src::engine::states::mirage {
namespace {

constexpr auto PICTURE_PATH = "assets/03C3.bmp";

constexpr int SCREEN_WIDTH = 368;
constexpr int SCREEN_HEIGHT = 290;
constexpr int DISPLAY_LINE = 30;
constexpr std::size_t SCREEN_COLORS = 32;
constexpr effects::AmigaColor BLACK = 0x000;

constexpr effects::FotoSequence::Timings TIMINGS{5, 200, 5, 70, true};

effects::AmigaPalette screenPalette(const systems::IndexedBitmap &picture) {
  effects::AmigaPalette palette = picture.palette;
  palette.resize(SCREEN_COLORS);
  return palette;
}

} // namespace

MirageState::MirageState(systems::VideoSystem &videoSystem)
    : m_videoSystem(videoSystem),
      m_rows(effects::visibleRows(DISPLAY_LINE, SCREEN_HEIGHT,
                                  videoSystem.isNtsc())),
      m_picture(systems::loadIndexedBitmap(PICTURE_PATH)),
      m_screen(SCREEN_WIDTH, m_rows.count),
      m_sequence(screenPalette(m_picture), TIMINGS) {}

std::optional<EngineStateEnum> MirageState::update() {
  if (m_sequence.isFinished()) {
    return EngineStateEnum::WorldSoftware;
  }

  m_sequence.advance();
  if (m_sequence.isShown()) {
    m_screen.setPalette(m_sequence.palette());
    m_screen.draw(m_picture, 0, -m_rows.first);
  } else {
    m_screen.fill(BLACK);
  }
  m_videoSystem.show(m_screen.output());
  return std::nullopt;
}

} // namespace openfranko::src::engine::states::mirage
