#include "KneeAnimationState.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace openfranko::src::engine::states::kneeAnimation {
namespace {

struct Image {
  const char *name;
  const char *path;
};

constexpr std::array<Image, 4> IMAGES = {{{"knee0", "assets/03B7/03B7.bmp"},
                                          {"knee1", "assets/03B7/03B7_1.bmp"},
                                          {"knee2", "assets/03B7/03B7_2.bmp"},
                                          {"knee3", "assets/03B7/03B7_3.bmp"}}};

constexpr auto SAMPLE = "knee";
constexpr auto SAMPLE_PATH = "assets/0263/0263_sam2_6453Hz.wav";
constexpr auto MUSIC_PATH = "assets/0261.s3m";

constexpr int SCREEN_ID = 2;
constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 256;
constexpr uint8_t BACKGROUND_GREY = 0x55;

constexpr int FRAMES_PER_UNPACK = 20;
constexpr int SAMPLE_UNPACK = 2;
constexpr int MUSIC_WAIT = 20;
constexpr int TEMPO_WAIT = 2;
constexpr int CLOSE_WAIT = 80;

constexpr int IMAGE_COUNT = static_cast<int>(IMAGES.size());
constexpr int SAMPLE_FRAME = SAMPLE_UNPACK * FRAMES_PER_UNPACK;
constexpr int MUSIC_FRAME = IMAGE_COUNT * FRAMES_PER_UNPACK + MUSIC_WAIT;
constexpr int CLOSE_FRAME = MUSIC_FRAME + TEMPO_WAIT + CLOSE_WAIT;
constexpr int SCREENS = 2;
constexpr int SCREEN_OPEN_VBLS = 1;
constexpr int SCREEN_CLOSE_VBLS = 2;
constexpr int OPEN_FRAMES = SCREENS * SCREEN_OPEN_VBLS;
constexpr int CLOSED_FRAME = CLOSE_FRAME + SCREENS * SCREEN_CLOSE_VBLS;

} // namespace

KneeAnimationState::KneeAnimationState(
    systems::VideoSystem &videoSystem, systems::AudioSystem &audioSystem,
    systems::ControllerSystem &controllerSystem)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem) {
  controllerSystem.clearFireLatch();
  m_videoSystem.createScreen(SCREEN_ID, SCREEN_WIDTH, SCREEN_HEIGHT);
  m_videoSystem.switchScreen(SCREEN_ID);
  for (const Image &image : IMAGES) {
    m_videoSystem.loadImage(image.name, image.path);
  }
  m_audioSystem.loadSFX(SAMPLE, SAMPLE_PATH);
  m_audioSystem.loadMusic(MUSIC_PATH);
}

KneeAnimationState::~KneeAnimationState() {
  m_audioSystem.clearSFX(SAMPLE);
  for (const Image &image : IMAGES) {
    m_videoSystem.clearImage(image.name);
  }
  m_videoSystem.fillScreen(0, 0, 0);
}

std::optional<EngineStateEnum> KneeAnimationState::update() {
  const int time = m_frame - OPEN_FRAMES;
  if (time == CLOSED_FRAME) {
    return EngineStateEnum::TitleAndStory;
  }

  if (time == SAMPLE_FRAME) {
    m_audioSystem.playSFX(SAMPLE);
  }
  if (time == MUSIC_FRAME) {
    m_audioSystem.playMusic();
  }

  const int copied = std::min(time / FRAMES_PER_UNPACK, IMAGE_COUNT);
  if (time < 0 || time >= CLOSE_FRAME) {
    m_videoSystem.fillScreen(0, 0, 0);
  } else if (copied == 0) {
    m_videoSystem.fillScreen(BACKGROUND_GREY, BACKGROUND_GREY, BACKGROUND_GREY);
  } else {
    m_videoSystem.drawImage(IMAGES[copied - 1].name, 0, 0);
  }

  ++m_frame;
  return std::nullopt;
}

} // namespace openfranko::src::engine::states::kneeAnimation
