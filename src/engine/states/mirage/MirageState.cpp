#include "MirageState.h"

#include <cstddef>

namespace openfranko::src::engine::states::mirage {
namespace {

constexpr auto PICTURE = "mirage";
constexpr auto PICTURE_PATH = "assets/03C3.bmp";

constexpr int SCREEN_ID = 0;
constexpr int SCREEN_WIDTH = 368;
constexpr int SCREEN_HEIGHT = 290;
constexpr std::size_t SCREEN_COLORS = 32;

constexpr effects::FotoSequence::Timings TIMINGS{5, 200, 5, 70, true};

effects::AmigaPalette openScreen(systems::VideoSystem &videoSystem) {
  videoSystem.createScreen(SCREEN_ID, SCREEN_WIDTH, SCREEN_HEIGHT);
  videoSystem.switchScreen(SCREEN_ID);
  videoSystem.loadIndexedImage(PICTURE, PICTURE_PATH);

  auto palette = videoSystem.getImagePalette(PICTURE);
  palette.resize(SCREEN_COLORS);
  return palette;
}

} // namespace

MirageState::MirageState(systems::VideoSystem &videoSystem)
    : m_videoSystem(videoSystem), m_sequence(openScreen(videoSystem), TIMINGS) {
  m_videoSystem.setImagePalette(PICTURE, m_sequence.palette());
}

MirageState::~MirageState() {
  m_videoSystem.clearImage(PICTURE);
  m_videoSystem.fillScreen(0, 0, 0);
}

std::optional<EngineStateEnum> MirageState::update() {
  if (m_sequence.isFinished()) {
    return EngineStateEnum::WorldSoftware;
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

} // namespace openfranko::src::engine::states::mirage
