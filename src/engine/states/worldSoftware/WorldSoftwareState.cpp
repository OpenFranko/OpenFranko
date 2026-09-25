#include "WorldSoftwareState.h"

#include <cstddef>

namespace openfranko::src::engine::states::worldSoftware {
namespace {

constexpr auto PICTURE = "worldSoftware";
constexpr auto PICTURE_PATH = "assets/03B6.bmp";

constexpr auto SAMPLE = "worldSoftware";
constexpr auto SAMPLE_PATH = "assets/0263/0263_sam1_13160Hz.wav";

constexpr int SCREEN_ID = 0;
constexpr int SCREEN_WIDTH = 368;
constexpr int SCREEN_HEIGHT = 290;
constexpr std::size_t SCREEN_COLORS = 32;

constexpr effects::FotoSequence::Timings TIMINGS{5, 200, 5, 75, false};

constexpr std::size_t EYES_COLOR = 22;
const effects::FlashSteps EYES_FLASH = {
    {0xF00, 4}, {0xE00, 4}, {0xD00, 4}, {0xC00, 4}, {0xB00, 4},
    {0xA00, 4}, {0x900, 4}, {0x800, 4}, {0x900, 4}, {0xA00, 4},
    {0xB00, 4}, {0xC00, 4}, {0xD00, 4}, {0xE00, 4}};

effects::AmigaPalette openScreen(systems::VideoSystem &videoSystem) {
  videoSystem.createScreen(SCREEN_ID, SCREEN_WIDTH, SCREEN_HEIGHT);
  videoSystem.switchScreen(SCREEN_ID);
  videoSystem.loadIndexedImage(PICTURE, PICTURE_PATH);

  auto palette = videoSystem.getImagePalette(PICTURE);
  palette.resize(SCREEN_COLORS);
  return palette;
}

} // namespace

WorldSoftwareState::WorldSoftwareState(systems::VideoSystem &videoSystem,
                                       systems::AudioSystem &audioSystem)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_sequence(openScreen(videoSystem), TIMINGS) {
  m_videoSystem.setImagePalette(PICTURE, m_sequence.palette());
  m_audioSystem.loadSFX(SAMPLE, SAMPLE_PATH);
}

WorldSoftwareState::~WorldSoftwareState() {
  m_audioSystem.clearSFX(SAMPLE);
  m_videoSystem.clearImage(PICTURE);
  m_videoSystem.fillScreen(0, 0, 0);
}

std::optional<EngineStateEnum> WorldSoftwareState::update() {
  if (m_sequence.isFinished()) {
    return EngineStateEnum::KneeAnimation;
  }

  if (m_sequence.frame() == m_sequence.holdStart()) {
    m_sequence.flash(EYES_COLOR, EYES_FLASH);
    m_audioSystem.playSample(SAMPLE, systems::AudioSystem::ALL_VOICES);
  }

  if (m_sequence.advance()) {
    m_videoSystem.setImagePalette(PICTURE, m_sequence.palette());
  }
  if (m_sequence.isShown()) {
    m_videoSystem.drawImage(PICTURE, 0, 0);
  } else {
    m_videoSystem.fillScreen(0, 0, 0);
  }
  return std::nullopt;
}

} // namespace openfranko::src::engine::states::worldSoftware
