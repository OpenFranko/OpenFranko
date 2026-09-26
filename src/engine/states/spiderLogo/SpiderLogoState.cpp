#include "SpiderLogoState.h"

#include "../../assets/Assets.h"

#include <cstddef>

namespace openfranko::src::engine::states::spiderLogo {
namespace {

constexpr auto BOBS = "s50";
constexpr auto LOGO = "p50";
constexpr auto TUNE = "m11";
constexpr int IMAGES = 10;

constexpr auto STEP_SAMPLE = "spiderStep";
constexpr auto JINGLE_SAMPLE = "spiderJingle";
constexpr int STEP_SAMPLE_NUMBER = 1;
constexpr int JINGLE_SAMPLE_NUMBER = 2;
constexpr int STEP_VOICES = 0x1;
constexpr int JINGLE_VOICES = 0x3;

constexpr int WALK_WIDTH = 688;
constexpr int WALK_HEIGHT = 280;
constexpr int WALK_DISPLAY_LINE = 42;
constexpr int WATER_TOP = 180;
constexpr uint8_t WATER_COLOR = 13;
constexpr int LOGO_WIDTH = 320;
constexpr int LOGO_HEIGHT = 256;
constexpr int LOGO_DISPLAY_LINE = 42;
constexpr int REFLECTION_TOP = 133;
constexpr int REFLECTION_BOTTOM = 200;
constexpr std::size_t LOGO_COLORS = 16;
constexpr effects::AmigaColor BLACK = 0x000;

const effects::AmigaPalette WALK_PALETTE = {
    0x000, 0x600, 0x333, 0x550, 0x444, 0x770, 0x008, 0x009,
    0x00A, 0x00B, 0x00C, 0x00D, 0x222, 0x003, 0xFFF, 0xFFF};

constexpr int BOB_CHANNEL = 1;
constexpr int16_t WALK_X = 770;
constexpr int16_t WALK_Y = 184;
constexpr int16_t WALK_IMAGE = 1;
constexpr auto WALK_PROGRAM =
    "FR1=0T15;LA=1;LX=X-16;M0,0,4;LA=2;LR0=1;LX=X-8;M0,0,4;LA=3;LX=X-16;M0,0,"
    "4;LA=4;LR0=1;LX=X-16;M0,0,4;NR1;";
constexpr int16_t REFLECTION_X = 158;
constexpr int16_t REFLECTION_Y = 133;
constexpr int16_t REFLECTION_IMAGE = 7;
constexpr auto REFLECTION_PROGRAM =
    "A 0,(5,12)(6,12)(7,12)(8,12)(9,12)(10,12);";

constexpr int DOUBLE_BUFFER_VBLS = 3;
constexpr int AUTOBACK_VBLS = 3;
constexpr int WALK_SETUP =
    effects::SCREEN_REOPEN_VBLS + DOUBLE_BUFFER_VBLS + 2 * AUTOBACK_VBLS - 1;
constexpr int TEMPO_WAIT = 2;
constexpr int STEP_TIMER = 10;
constexpr int WALK_TEMPO = 14;
constexpr int LOGO_SETUP =
    effects::SCREEN_OPEN_VBLS + DOUBLE_BUFFER_VBLS + AUTOBACK_VBLS - 1;
constexpr int JINGLE_WAIT = 10;
constexpr effects::FotoSequence::Timings LOGO_TIMINGS{3, 210, 3, 45, false};

systems::IndexedBitmap filled(int width, int height, uint8_t color) {
  systems::IndexedBitmap bitmap;
  bitmap.width = width;
  bitmap.height = height;
  bitmap.pixels.assign(static_cast<std::size_t>(width) * height, color);
  return bitmap;
}

effects::AmigaPalette logoPalette(const systems::IndexedBitmap &logo) {
  effects::AmigaPalette palette = logo.palette;
  palette.resize(LOGO_COLORS);
  return palette;
}

} // namespace

SpiderLogoState::SpiderLogoState(systems::VideoSystem &videoSystem,
                                 systems::AudioSystem &audioSystem)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_logo(systems::loadIndexedBitmap(assets::picturePath(LOGO))),
      m_water(filled(WALK_WIDTH, WALK_HEIGHT - WATER_TOP, WATER_COLOR)),
      m_reflectionArea(
          filled(LOGO_WIDTH, REFLECTION_BOTTOM - REFLECTION_TOP, 0)),
      m_machine(m_registers),
      m_walkRows(effects::visibleRows(WALK_DISPLAY_LINE, WALK_HEIGHT,
                                      videoSystem.isNtsc())),
      m_logoRows(effects::visibleRows(LOGO_DISPLAY_LINE, LOGO_HEIGHT,
                                      videoSystem.isNtsc())),
      m_walkScreen(WALK_WIDTH, m_walkRows.count),
      m_logoScreen(LOGO_WIDTH, m_logoRows.count) {
  for (int index = 0; index < IMAGES; ++index) {
    m_images.push_back(
        systems::loadIndexedBitmap(assets::imagePath(BOBS, index)));
  }
  m_audioSystem.loadSFX(STEP_SAMPLE,
                        assets::samplePath(BOBS, STEP_SAMPLE_NUMBER));
  m_audioSystem.loadSFX(JINGLE_SAMPLE,
                        assets::samplePath(BOBS, JINGLE_SAMPLE_NUMBER));
  m_audioSystem.loadMusic(assets::musicPath(TUNE));
  m_machine.bind(BOB_CHANNEL, &m_bob);
}

SpiderLogoState::~SpiderLogoState() {
  m_audioSystem.clearSFX(STEP_SAMPLE);
  m_audioSystem.clearSFX(JINGLE_SAMPLE);
}

std::optional<EngineStateEnum> SpiderLogoState::update() {
  if (m_foto && m_foto->isFinished()) {
    m_audioSystem.stopMusic();
    return EngineStateEnum::Adverts;
  }
  if (m_logoStart) {
    logo();
  } else {
    walk();
  }
  ++m_frame;
  return std::nullopt;
}

void SpiderLogoState::walk() {
  if (m_frame < WALK_SETUP) {
    showBlack(m_walkScreen, true);
    return;
  }
  if (m_frame == WALK_SETUP) {
    m_bob = {WALK_X, WALK_Y, WALK_IMAGE};
    m_machine.create(BOB_CHANNEL, WALK_PROGRAM);
    m_machine.startAll();
    m_timer = 0;
    m_audioSystem.playMusicOnce();
    showBlack(m_walkScreen, true);
    return;
  }

  m_shownBob = m_bob;
  m_machine.tick();
  ++m_timer;
  if (m_frame == WALK_SETUP + TEMPO_WAIT) {
    m_audioSystem.setMusicTempo(WALK_TEMPO);
  }
  if (m_frame >= WALK_SETUP + TEMPO_WAIT && !m_walkEnd) {
    if (!m_machine.isRunning(BOB_CHANNEL)) {
      m_walkEnd = m_frame;
    } else if (m_timer > STEP_TIMER) {
      m_audioSystem.playSample(STEP_SAMPLE, STEP_VOICES);
      m_timer = 0;
    }
  }

  if (!m_walkEnd || m_frame < *m_walkEnd + effects::SCREEN_CLOSE_SHOWN_VBLS) {
    showWalk();
  } else {
    showBlack(m_walkScreen, true);
  }
  if (m_walkEnd && m_frame == *m_walkEnd + effects::SCREEN_CLOSE_VBLS) {
    m_logoStart = m_frame + 1;
    m_machine.destroyAll();
  }
}

void SpiderLogoState::logo() {
  const int time = m_frame - *m_logoStart;
  if (time == LOGO_SETUP) {
    m_bob = {REFLECTION_X, REFLECTION_Y, REFLECTION_IMAGE};
    m_machine.create(BOB_CHANNEL, REFLECTION_PROGRAM);
    m_machine.startAll();
    m_foto.emplace(logoPalette(m_logo), LOGO_TIMINGS);
  } else if (time > LOGO_SETUP) {
    m_shownBob = m_bob;
    m_machine.tick();
  }
  if (!m_foto) {
    showBlack(m_logoScreen, false);
    return;
  }
  if (m_foto->frame() == m_foto->holdStart() + JINGLE_WAIT) {
    m_audioSystem.playSample(JINGLE_SAMPLE, JINGLE_VOICES);
  }
  m_foto->advance();
  if (m_foto->isShown()) {
    showLogo();
  } else {
    showBlack(m_logoScreen, false);
  }
}

void SpiderLogoState::showWalk() {
  m_walkScreen.fill(BLACK);
  m_walkScreen.setPalette(WALK_PALETTE);
  m_walkScreen.draw(m_water, 0, WATER_TOP - m_walkRows.first);
  drawBob(m_walkScreen, m_walkRows.first);
  systems::Display display = m_walkScreen.output();
  display.displayHeight = 2 * display.height;
  m_videoSystem.show(display);
}

void SpiderLogoState::showLogo() {
  m_logoScreen.setPalette(m_foto->palette());
  m_logoScreen.draw(m_logo, 0, -m_logoRows.first);
  m_logoScreen.draw(m_reflectionArea, 0, REFLECTION_TOP - m_logoRows.first);
  drawBob(m_logoScreen, m_logoRows.first);
  m_videoSystem.show(m_logoScreen.output());
}

void SpiderLogoState::showBlack(systems::Canvas &screen, bool hires) {
  screen.fill(BLACK);
  systems::Display display = screen.output();
  if (hires) {
    display.displayHeight = 2 * display.height;
  }
  m_videoSystem.show(display);
}

void SpiderLogoState::drawBob(systems::Canvas &screen, int top) const {
  const int image = m_shownBob.image - 1;
  if (image >= 0 && image < static_cast<int>(m_images.size())) {
    screen.drawMasked(m_images[static_cast<std::size_t>(image)], m_shownBob.x,
                      m_shownBob.y - top);
  }
}

} // namespace openfranko::src::engine::states::spiderLogo
