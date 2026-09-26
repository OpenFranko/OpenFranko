#include "GameOverScene.h"

#include "../effects/AmigaDisplay.h"

#include "../effects/Rainbow.h"
#include "StageFrame.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace openfranko::src::engine::street {
namespace {

constexpr int OBJECTS = 0x36;
constexpr int GRAVEYARD = 0x3BB;
constexpr int GAME_OVER_TUNE = 0x262;

constexpr int FULL_VOLUME = 63;
constexpr effects::AmigaColor BLACK = 0x000;

constexpr int TITLE = 1;
constexpr int HAND = 2;
constexpr int TITLE_IMAGE = 6;
constexpr int HAND_IMAGE = 1;
constexpr int WINDOW_X = 104 & ~15;
constexpr int PINNED_X = 200;
constexpr int TITLE_Y = 80;
constexpr int HAND_X = 820;
constexpr int HAND_Y = 209;

constexpr int SCREEN_TOP = GameOverScene::DISPLAY_LINE;
constexpr int RAINBOW_ENTRIES = 1000;
constexpr int RAINBOW_BASE = 96;
constexpr int RAINBOW_Y = 0;
constexpr int FIRST_RAINBOW_LINE = 28;
constexpr int RAINBOW_LINES = 240;

constexpr int PAN_STEP_PIXELS = 5;
constexpr int PAN_STEP_FRAMES = 4;
constexpr int CLICK_FRAMES = 400;
constexpr int16_t CLICK_FIRE = 16;
constexpr int UNPACK_VBLS = 1;
constexpr int DOUBLE_BUFFER_VBLS = 3;
constexpr int FADE_SPEED = 5;
constexpr int HOLD_FRAMES = 100;

const std::vector<effects::AmalAnim::Frame> HAND_FRAMES = {
    {1, 5}, {2, 5}, {3, 5}, {4, 5}, {5, 25}, {3, 5}, {2, 5}, {1, 25}};

effects::AmigaPalette graveyardPalette() {
  effects::AmigaPalette palette(32, BLACK);
  palette[2] = 0xF00;
  palette[9] = 0x222;
  return palette;
}

} // namespace

GameOverScene::GameOverScene(StreetHost &host, GameSession &session)
    : m_host(host), m_session(session), m_screen(PICTURE_WIDTH, PICTURE_HEIGHT),
      m_palette(graveyardPalette()), m_border(session.border),
      m_copperBorder(session.border) {}

void GameOverScene::advance(int16_t joystick) {
  if (m_step == Step::Finished) {
    return;
  }
  ++m_frame;
  if (m_buffer) {
    m_buffer->vbl();
  }
  m_shown = m_copperShown;
  m_border = m_copperBorder;
  m_shownOffset = m_copperOffset;
  m_copperOffset = m_offset;
  if (m_animating) {
    m_bobs.setImage(HAND, m_hand.advance(m_bobs.image(HAND)));
  }
  m_fader.tick(m_palette);
  if (m_buffer && !holdsAtStart()) {
    m_buffer->test(m_bobs, m_images);
  }

  Flow flow = Flow::Continue;
  while (flow == Flow::Continue && m_frame >= m_resumeFrame) {
    switch (m_step) {
    case Step::Close:
      close();
      m_step = Step::Loading;
      break;
    case Step::Loading:
      if (m_loading.advance(nullptr)) {
        m_step = Step::Open;
      } else {
        flow = Flow::Yield;
      }
      break;
    case Step::Open:
      unpack();
      flow = wait(UNPACK_VBLS, Step::Unpacked);
      break;
    case Step::Unpacked:
      m_buffer.emplace(m_screen);
      flow = wait(DOUBLE_BUFFER_VBLS, Step::Opened);
      break;
    case Step::Opened:
      open();
      m_count = 0;
      m_step = Step::Pan;
      break;
    case Step::Pan:
      flow = pan();
      break;
    case Step::Click:
      flow = click(joystick);
      break;
    case Step::MusicFade:
      flow = musicFade();
      break;
    case Step::Hold:
      flow = hold();
      break;
    case Step::CloseShown:
      closeGraveyard();
      flow = wait(effects::SCREEN_CLOSE_HIDDEN_VBLS, Step::Closed);
      break;
    case Step::Closed:
      m_step = Step::Finished;
      flow = Flow::Yield;
      break;
    case Step::Finished:
      flow = Flow::Yield;
      break;
    }
  }
  if (m_buffer && !holdsAtEnd()) {
    m_buffer->test(m_bobs, m_images);
  }
}

void GameOverScene::compose(std::vector<uint32_t> &frame) const {
  systems::rasterize(output(), frame);
}

systems::Display GameOverScene::output() const {
  systems::Display display;
  display.width = WIDTH;
  display.height = HEIGHT;
  display.displayHeight = HEIGHT;
  display.border = m_border;
  if (!m_shown || !m_buffer) {
    return display;
  }
  const IndexedSurface &shown = m_buffer->shown();
  systems::Layer layer;
  layer.pixels = shown.pixels().data();
  layer.stride = shown.width();
  layer.sourceColumns = PICTURE_WIDTH;
  layer.sourceRows = PICTURE_HEIGHT;
  layer.sourceX = m_shownOffset;
  layer.wrap = true;
  layer.columns = WIDTH;
  layer.rows = HEIGHT;
  layer.palette = m_palette;
  const int top = std::max(RAINBOW_Y, FIRST_RAINBOW_LINE);
  const int size = static_cast<int>(m_rainbow.size());
  for (int row = 0; m_rainbowShown && size > 0 && row < HEIGHT; ++row) {
    const int line = SCREEN_TOP + row;
    if (line >= top && line < top + RAINBOW_LINES) {
      layer.rowColors.push_back({row, 0,
                                 m_rainbow[static_cast<std::size_t>(
                                     (RAINBOW_BASE + line - top) % size)]});
    }
  }
  display.layers.push_back(std::move(layer));
  return display;
}

bool GameOverScene::isShown() const { return m_shown; }

bool GameOverScene::isPanning() const { return m_step == Step::Pan; }

bool GameOverScene::isFinished() const { return m_step == Step::Finished; }

int GameOverScene::offset() const { return m_offset; }

const BobLayer &GameOverScene::bobs() const { return m_bobs; }

const effects::AmigaPalette &GameOverScene::palette() const {
  return m_palette;
}

void GameOverScene::close() {
  m_host.stopMusic();
  m_images.clear();
  m_shown = false;
  m_copperShown = false;
  m_loading.queue(
      [this] { m_images.load(1, m_host.loadSpriteSet(OBJECTS, 0)); });
  m_loading.queue([this] { m_picture = m_host.loadPicture(GRAVEYARD); });
  m_loading.queue([this] { m_host.loadMusic(GAME_OVER_TUNE); });
}

GameOverScene::Flow GameOverScene::wait(int frames, Step next) {
  m_holdStart = m_frame;
  m_holdUntil = m_frame + frames;
  m_resumeFrame = m_frame + frames;
  m_step = next;
  return Flow::Yield;
}

bool GameOverScene::holdsAtStart() const {
  return m_holdStart < m_frame && m_frame <= m_holdUntil;
}

bool GameOverScene::holdsAtEnd() const {
  return m_holdStart <= m_frame && m_frame < m_holdUntil;
}

void GameOverScene::unpack() {
  m_host.setMusicVolume(FULL_VOLUME);
  m_host.playMusic();
  m_screen.unpack(m_picture, 0, 0);
  m_picture = Picture{};
}

void GameOverScene::open() {
  m_palette = graveyardPalette();
  m_rainbow = effects::rainbowTable(RAINBOW_ENTRIES, "(8,-1,15)(16,1,15)", "",
                                    "(8,1,15)(16,-1,15)");
  m_rainbowShown = true;
  m_bobs.set(HAND, HAND_X, HAND_Y, HAND_IMAGE);
  m_bobs.set(TITLE, PINNED_X, TITLE_Y, TITLE_IMAGE);
  m_hand = effects::AmalAnim(HAND_FRAMES, 0);
  m_animating = true;
  m_copperShown = true;
  m_copperOffset = m_offset;
  m_copperBorder = BLACK;
  m_session.border = BLACK;
  m_buffer->test(m_bobs, m_images);
}

GameOverScene::Flow GameOverScene::pan() {
  const int offset = m_count * PAN_STEP_PIXELS / PAN_STEP_FRAMES;
  if (offset > PAN_END) {
    m_count = 0;
    m_step = Step::Click;
    return Flow::Continue;
  }
  m_offset = offset;
  m_bobs.setX(TITLE, PINNED_X - WINDOW_X + m_offset);
  ++m_count;
  return Flow::Yield;
}

GameOverScene::Flow GameOverScene::click(int16_t joystick) {
  ++m_count;
  const bool pressed = m_session.version == GameVersion::V12
                           ? (joystick & CLICK_FIRE) != 0
                           : joystick > 0;
  if (m_count <= CLICK_FRAMES && !pressed) {
    return Flow::Yield;
  }
  m_fader.start(m_palette, FADE_SPEED,
                effects::AmigaPalette(m_palette.size(), BLACK));
  m_count = FULL_VOLUME;
  m_step = Step::MusicFade;
  return Flow::Continue;
}

GameOverScene::Flow GameOverScene::musicFade() {
  if (m_count < 0) {
    m_host.stopMusic();
    m_host.setMusicVolume(FULL_VOLUME);
    m_count = HOLD_FRAMES;
    m_step = Step::Hold;
    return Flow::Yield;
  }
  m_host.setMusicVolume(m_count);
  --m_count;
  return Flow::Yield;
}

GameOverScene::Flow GameOverScene::hold() {
  if (--m_count > 0) {
    return Flow::Yield;
  }
  m_animating = false;
  return wait(effects::SCREEN_CLOSE_SHOWN_VBLS, Step::CloseShown);
}

void GameOverScene::closeGraveyard() {
  m_rainbowShown = false;
  m_bobs.offAll();
  m_shown = false;
  m_copperShown = false;
}

} // namespace openfranko::src::engine::street
