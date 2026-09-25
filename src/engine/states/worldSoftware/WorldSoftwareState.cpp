#include "WorldSoftwareState.h"

#include <cstddef>

namespace openfranko::src::engine::states::worldSoftware {
namespace {

constexpr auto PICTURE_PATH = "assets/03B6.bmp";

constexpr auto SAMPLE = "worldSoftware";
constexpr auto SAMPLE_PATH = "assets/0263/0263_sam1_13160Hz.wav";

constexpr int SCREEN_WIDTH = 368;
constexpr int SCREEN_HEIGHT = 290;
constexpr int DISPLAY_LINE = 25;
constexpr std::size_t SCREEN_COLORS = 32;
constexpr effects::AmigaColor BLACK = 0x000;

constexpr effects::FotoSequence::Timings TIMINGS{5, 200, 5, 75, false};

constexpr std::size_t EYES_COLOR = 22;
const effects::FlashSteps EYES_FLASH = {
    {0xF00, 4}, {0xE00, 4}, {0xD00, 4}, {0xC00, 4}, {0xB00, 4},
    {0xA00, 4}, {0x900, 4}, {0x800, 4}, {0x900, 4}, {0xA00, 4},
    {0xB00, 4}, {0xC00, 4}, {0xD00, 4}, {0xE00, 4}};

effects::AmigaPalette screenPalette(const systems::IndexedBitmap &picture) {
  effects::AmigaPalette palette = picture.palette;
  palette.resize(SCREEN_COLORS);
  return palette;
}

} // namespace

WorldSoftwareState::WorldSoftwareState(systems::VideoSystem &videoSystem,
                                       systems::AudioSystem &audioSystem)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_rows(effects::visibleRows(DISPLAY_LINE, SCREEN_HEIGHT,
                                  videoSystem.isNtsc())),
      m_picture(systems::loadIndexedBitmap(PICTURE_PATH)),
      m_screen(SCREEN_WIDTH, m_rows.count),
      m_sequence(screenPalette(m_picture), TIMINGS) {
  m_audioSystem.loadSFX(SAMPLE, SAMPLE_PATH);
}

WorldSoftwareState::~WorldSoftwareState() { m_audioSystem.clearSFX(SAMPLE); }

std::optional<EngineStateEnum> WorldSoftwareState::update() {
  if (m_sequence.isFinished()) {
    return EngineStateEnum::KneeAnimation;
  }

  if (m_sequence.frame() == m_sequence.holdStart()) {
    m_sequence.flash(EYES_COLOR, EYES_FLASH);
    m_audioSystem.playSample(SAMPLE, systems::AudioSystem::ALL_VOICES);
  }

  m_sequence.advance();
  if (m_sequence.isShown()) {
    m_screen.draw(m_picture, m_sequence.palette(), 0, -m_rows.first);
  } else {
    m_screen.fill(BLACK);
  }
  m_videoSystem.show(m_screen.pixels().data(), m_screen.width(),
                     m_screen.height());
  return std::nullopt;
}

} // namespace openfranko::src::engine::states::worldSoftware
