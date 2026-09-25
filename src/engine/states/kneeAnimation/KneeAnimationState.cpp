#include "KneeAnimationState.h"

#include "../../effects/AmigaDisplay.h"
#include "../../effects/AmigaPalette.h"

#include <algorithm>
#include <array>

namespace openfranko::src::engine::states::kneeAnimation {
namespace {

constexpr std::array<const char *, 4> IMAGE_PATHS = {
    {"assets/03B7/03B7.bmp", "assets/03B7/03B7_1.bmp", "assets/03B7/03B7_2.bmp",
     "assets/03B7/03B7_3.bmp"}};

constexpr auto SAMPLE = "knee";
constexpr auto SAMPLE_PATH = "assets/0263/0263_sam2_6453Hz.wav";
constexpr auto MUSIC_PATH = "assets/0261.s3m";

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 256;
constexpr effects::AmigaColor BACKGROUND_GREY = 0x555;
constexpr effects::AmigaColor BLACK = 0x000;

constexpr int FRAMES_PER_UNPACK = 20;
constexpr int SAMPLE_UNPACK = 2;
constexpr int MUSIC_WAIT = 20;
constexpr int TEMPO_WAIT = 2;
constexpr int CLOSE_WAIT = 80;

constexpr int IMAGE_COUNT = static_cast<int>(IMAGE_PATHS.size());
constexpr int SAMPLE_FRAME = SAMPLE_UNPACK * FRAMES_PER_UNPACK;
constexpr int MUSIC_FRAME = IMAGE_COUNT * FRAMES_PER_UNPACK + MUSIC_WAIT;
constexpr int CLOSE_FRAME = MUSIC_FRAME + TEMPO_WAIT + CLOSE_WAIT;
constexpr int SCREENS = 2;
constexpr int OPEN_FRAMES = SCREENS * effects::SCREEN_OPEN_VBLS;
constexpr int GONE_FRAME = CLOSE_FRAME + effects::SCREEN_CLOSE_SHOWN_VBLS;
constexpr int CLOSED_FRAME = CLOSE_FRAME + SCREENS * effects::SCREEN_CLOSE_VBLS;

} // namespace

KneeAnimationState::KneeAnimationState(
    systems::VideoSystem &videoSystem, systems::AudioSystem &audioSystem,
    systems::ControllerSystem &controllerSystem)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_screen(SCREEN_WIDTH, SCREEN_HEIGHT) {
  controllerSystem.clearFireLatch();
  for (const char *path : IMAGE_PATHS) {
    m_images.push_back(systems::loadIndexedBitmap(path));
  }
  m_audioSystem.loadSFX(SAMPLE, SAMPLE_PATH);
  m_audioSystem.loadMusic(MUSIC_PATH);
}

KneeAnimationState::~KneeAnimationState() { m_audioSystem.clearSFX(SAMPLE); }

std::optional<EngineStateEnum> KneeAnimationState::update() {
  const int time = m_frame - OPEN_FRAMES;
  if (time == CLOSED_FRAME) {
    return EngineStateEnum::TitleAndStory;
  }

  if (time == SAMPLE_FRAME) {
    m_audioSystem.playSample(SAMPLE, systems::AudioSystem::ALL_VOICES);
  }
  if (time == MUSIC_FRAME) {
    m_audioSystem.playMusic();
  }

  const int copied = std::min(time / FRAMES_PER_UNPACK, IMAGE_COUNT);
  if (time < 0 || time >= GONE_FRAME) {
    m_screen.fill(BLACK);
  } else if (copied == 0) {
    m_screen.fill(BACKGROUND_GREY);
  } else {
    const systems::IndexedBitmap &image = m_images[copied - 1];
    m_screen.setPalette(image.palette);
    m_screen.draw(image, 0, 0);
  }
  m_videoSystem.show(m_screen.output());

  ++m_frame;
  return std::nullopt;
}

} // namespace openfranko::src::engine::states::kneeAnimation
