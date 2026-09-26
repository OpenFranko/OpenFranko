#include "KneeAnimationState.h"

#include "../../assets/Assets.h"
#include "../../effects/AmigaDisplay.h"
#include "../../effects/AmigaPalette.h"

#include <algorithm>
#include <string>

namespace openfranko::src::engine::states::kneeAnimation {
namespace {

constexpr int IMAGES = 0x3B7;
constexpr int MENU_TUNE = 0x261;

constexpr auto SAMPLE = "knee";
constexpr int SAMPLE_BANK = 0x263;
constexpr int SAMPLE_NUMBER = 2;
constexpr auto VERSION12_SAMPLE_BANK = "s50";
constexpr int VERSION12_SAMPLE_NUMBER = 3;
constexpr int VERSION12_SAMPLE_VOICES = 0x3;
constexpr int VERSION12_TEMPO = 37;

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 256;
constexpr effects::AmigaColor BACKGROUND_GREY = 0x555;
constexpr effects::AmigaColor BLACK = 0x000;

constexpr int FRAMES_PER_UNPACK = 20;
constexpr int SAMPLE_UNPACK = 2;
constexpr int MUSIC_WAIT = 20;
constexpr int TEMPO_WAIT = 2;
constexpr int CLOSE_WAIT = 80;

constexpr int IMAGE_COUNT = 4;
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
    systems::ControllerSystem &controllerSystem, GameVersion version)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_version(version), m_screen(SCREEN_WIDTH, SCREEN_HEIGHT) {
  const bool version12 = m_version == GameVersion::V12;
  if (!version12) {
    controllerSystem.clearFireLatch();
  }
  const std::string images = assets::resourceName(IMAGES, m_version);
  for (int image = 0; image < IMAGE_COUNT; ++image) {
    m_images.push_back(
        systems::loadIndexedBitmap(assets::partPath(images, image)));
  }
  m_audioSystem.loadSFX(
      SAMPLE,
      version12
          ? assets::samplePath(VERSION12_SAMPLE_BANK, VERSION12_SAMPLE_NUMBER)
          : assets::samplePath(assets::resourceName(SAMPLE_BANK, m_version),
                               SAMPLE_NUMBER));
  m_audioSystem.loadMusic(
      assets::musicPath(assets::resourceName(MENU_TUNE, m_version)));
}

KneeAnimationState::~KneeAnimationState() { m_audioSystem.clearSFX(SAMPLE); }

std::optional<EngineStateEnum> KneeAnimationState::update() {
  const int time = m_frame - OPEN_FRAMES;
  if (time == CLOSED_FRAME) {
    return EngineStateEnum::TitleAndStory;
  }

  const bool version12 = m_version == GameVersion::V12;
  if (time == SAMPLE_FRAME) {
    m_audioSystem.playSample(SAMPLE, version12
                                         ? VERSION12_SAMPLE_VOICES
                                         : systems::AudioSystem::ALL_VOICES);
  }
  if (time == MUSIC_FRAME) {
    m_audioSystem.playMusic();
  }
  if (version12 && time == MUSIC_FRAME + TEMPO_WAIT) {
    m_audioSystem.setMusicTempo(VERSION12_TEMPO);
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
