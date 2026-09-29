#include "WorldSoftwareState.h"

#include <cstddef>

namespace openfranko::src::engine::states::worldSoftware {
namespace {

constexpr auto PICTURE_PATH = "assets/03B6.bmp";

constexpr auto SAMPLE = "worldSoftware";
constexpr auto SAMPLE_PATH = "assets/0263/0263_sam1_13160Hz.wav";

constexpr int DISPLAY_LINE = 25;

constexpr effects::sequences::FotoSequence ::Timings TIMINGS{5, 200, 5, 75,
                                                             false};

constexpr std::size_t EYES_COLOR = 22;
const effects::color::FlashSteps EYES_FLASH = {
    {0xF00, 4}, {0xE00, 4}, {0xD00, 4}, {0xC00, 4}, {0xB00, 4},
    {0xA00, 4}, {0x900, 4}, {0x800, 4}, {0x900, 4}, {0xA00, 4},
    {0xB00, 4}, {0xC00, 4}, {0xD00, 4}, {0xE00, 4}};

} // namespace

WorldSoftwareState::WorldSoftwareState(
    systems::graphics::VideoSystem &videoSystem,
    systems::audio::AudioSystem &audioSystem)
    : m_audioSystem(audioSystem),
      m_foto(videoSystem, PICTURE_PATH, DISPLAY_LINE, TIMINGS) {
  m_audioSystem.loadSample(SAMPLE, SAMPLE_PATH);
}

WorldSoftwareState::~WorldSoftwareState() { m_audioSystem.clearSample(SAMPLE); }

std::optional<EngineStateId> WorldSoftwareState::update() {
  effects::sequences::FotoSequence &sequence = m_foto.sequence();
  if (sequence.isFinished()) {
    return EngineStateId::KneeAnimation;
  }

  if (sequence.frame() == sequence.holdStart()) {
    sequence.flash(EYES_COLOR, EYES_FLASH);
    m_audioSystem.playSample(SAMPLE, systems::audio::AudioSystem::ALL_VOICES);
  }

  m_foto.advance();
  return std::nullopt;
}

} // namespace openfranko::src::engine::states::worldSoftware
