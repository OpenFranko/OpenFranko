#include "KneeAnimationState.h"

#include "../../../systems/audio/Mixer.h"
#include "../../AmigaDisplay.h"
#include "../../assets/Assets.h"
#include "../../effects/color/AmigaPalette.h"

#include <algorithm>
#include <string>

namespace openfranko::src::engine::states::kneeAnimation {
namespace {

constexpr int IMAGE_SET = 0x3B7;

constexpr auto SAMPLE = "knee";
constexpr int SAMPLE_BANK = 0x263;
constexpr int SAMPLE_NUMBER = 2;
constexpr auto VERSION12_SAMPLE_BANK = "s50";
constexpr int VERSION12_SAMPLE_NUMBER = 3;
constexpr int VERSION12_SAMPLE_VOICES = 0x3;
constexpr int VERSION12_TEMPO = 37;

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 256;
constexpr effects::color::AmigaColor BACKGROUND_GREY = 0x555;

constexpr int FRAMES_PER_UNPACK = 20;
constexpr int SAMPLE_UNPACK = 2;
constexpr int MUSIC_WAIT = 20;
constexpr int TEMPO_WAIT = 2;
constexpr int CLOSE_WAIT = 80;

constexpr int IMAGES = 4;
constexpr int PICTURE_LOAD_STEPS = 2;
constexpr int SAMPLE_FRAME = SAMPLE_UNPACK * FRAMES_PER_UNPACK;
constexpr int MUSIC_FRAME = IMAGES * FRAMES_PER_UNPACK + MUSIC_WAIT;
constexpr int CLOSE_FRAME = MUSIC_FRAME + TEMPO_WAIT + CLOSE_WAIT;
constexpr int SCREENS = 2;
constexpr int OPEN_FRAMES = SCREENS * SCREEN_OPEN_VBLS;
constexpr int GONE_FRAME = CLOSE_FRAME + SCREEN_CLOSE_SHOWN_VBLS;
constexpr int CLOSED_FRAME = CLOSE_FRAME + SCREENS * SCREEN_CLOSE_VBLS;

} // namespace

KneeAnimationState::KneeAnimationState(
    systems::graphics::Monitor &monitor, systems::audio::Speaker &speaker,
    systems::input::ControllerSystem &controllerSystem, assets::Files &files,
    GameVersion version)
    : m_monitor(monitor), m_speaker(speaker), m_version(version),
      m_screen(SCREEN_WIDTH, SCREEN_HEIGHT) {
  const bool version12 = m_version == GameVersion::V12;
  if (!version12) {
    controllerSystem.clearFireLatch();
  }
  const std::string images = assets::resourceName(IMAGE_SET, m_version);
  const std::string sample =
      version12
          ? assets::samplePath(files, VERSION12_SAMPLE_BANK,
                               VERSION12_SAMPLE_NUMBER)
          : assets::samplePath(files,
                               assets::resourceName(SAMPLE_BANK, m_version),
                               SAMPLE_NUMBER);
  m_images.resize(IMAGES);
  for (int image = 0; image < IMAGES; ++image) {
    m_imageLoads.push_back(m_loads.add(
        shared::bitmapStep(files, assets::partPath(images, image),
                           m_images[static_cast<std::size_t>(image)])));
    if (image == SAMPLE_UNPACK - 1) {
      m_sampleLoad = m_loads.add([this, sample] {
        m_speaker.loadSample(SAMPLE, sample);
        return true;
      });
    }
  }
  m_musicLoad = m_loads.add(shared::musicStep(
      files, m_speaker,
      assets::musicPath(assets::resourceName(assets::MENU_TUNE, m_version))));
  m_speaker.stopMusic();
}

KneeAnimationState::~KneeAnimationState() { m_speaker.clearSample(SAMPLE); }

std::optional<EngineStateId> KneeAnimationState::update() {
  const int time = m_frame - OPEN_FRAMES;
  if (time == CLOSED_FRAME) {
    return EngineStateId::TitleAndStory;
  }

  m_loads.step(m_loads.isDone(m_imageLoads.back()) ? 1 : PICTURE_LOAD_STEPS);
  const bool version12 = m_version == GameVersion::V12;
  if (time == SAMPLE_FRAME) {
    m_loads.finish(m_sampleLoad);
    m_speaker.playSample(SAMPLE, version12 ? VERSION12_SAMPLE_VOICES
                                           : systems::audio::Mixer::ALL_VOICES);
  }
  if (time == MUSIC_FRAME) {
    m_loads.finish(m_musicLoad);
    m_speaker.playMusic();
  }
  if (version12 && time == MUSIC_FRAME + TEMPO_WAIT) {
    m_speaker.setMusicTempo(VERSION12_TEMPO);
  }

  const int copied = std::min(time / FRAMES_PER_UNPACK, IMAGES);
  if (time < 0 || time >= GONE_FRAME) {
    m_screen.fill(effects::color::BLACK);
  } else if (copied == 0) {
    m_screen.fill(BACKGROUND_GREY);
  } else {
    m_loads.finish(m_imageLoads[static_cast<std::size_t>(copied - 1)]);
    const systems::graphics::IndexedBitmap &image = m_images[copied - 1];
    m_screen.setPalette(image.palette);
    m_screen.draw(image, 0, 0);
  }
  m_monitor.show(m_screen.output());

  ++m_frame;
  return std::nullopt;
}

} // namespace openfranko::src::engine::states::kneeAnimation
