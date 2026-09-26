#include "ContinueScene.h"

#include "../effects/AmigaDisplay.h"

#include "../amal/Actors.h"
#include "StageFrame.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace openfranko::src::engine::street {
namespace {

constexpr int RO = 14;
constexpr int RQ = 16;

constexpr int LETTER_SET = 0x35;
constexpr int FIRST_IMAGE = 1;
constexpr int HAND_IMAGE = 1;
constexpr int QUESTION_IMAGE = 41;
constexpr int QUESTION_X = 96;
constexpr int QUESTION_Y = 96;
constexpr int LEFT_X = 48;
constexpr int RIGHT_X = 268;
constexpr int HAND_Y = 124;
constexpr int WAGGLE_REGISTER = 1;
constexpr int MACH_WAIT = 40;
constexpr int VOICE_BANK = 10;
constexpr int ALL_VOICES = 0xF;

constexpr int16_t JOY_LEFT = 4;
constexpr int16_t JOY_RIGHT = 8;
constexpr int16_t JOY_FIRE = 16;

constexpr std::size_t COLORS = 32;
constexpr effects::AmigaColor BLACK = 0x000;

effects::AmigaPalette continuePalette() {
  effects::AmigaPalette palette(COLORS, BLACK);
  palette[0] = 0x707;
  palette[18] = 0xAAA;
  palette[24] = 0xDDD;
  palette[29] = 0x769;
  palette[30] = 0xB95;
  palette[31] = 0xFC0;
  return palette;
}

} // namespace

ContinueScene::ContinueScene(StreetHost &host, GameSession &session)
    : m_host(host), m_session(session), m_machine(session.registers),
      m_screen(WIDTH, HEIGHT), m_display(WIDTH, HEIGHT),
      m_palette(COLORS, BLACK) {
  const bool voices = session.version == GameVersion::V12;
  m_images.load(FIRST_IMAGE,
                host.loadSpriteSet(LETTER_SET, voices ? VOICE_BANK : 0));
}

void ContinueScene::advance(int16_t joystick) {
  if (m_step == Step::Finished) {
    return;
  }
  Flow flow = Flow::Continue;
  while (flow == Flow::Continue && m_frame >= m_resumeFrame) {
    switch (m_step) {
    case Step::Open:
      open();
      m_step = Step::Choose;
      break;
    case Step::Choose:
      flow = choose(joystick);
      break;
    case Step::Chosen:
      close();
      flow = Flow::Yield;
      break;
    case Step::Gone:
      m_bobs.offAll();
      m_shown = false;
      m_resumeFrame = m_frame + effects::SCREEN_CLOSE_HIDDEN_VBLS;
      m_step = Step::Closed;
      flow = Flow::Yield;
      break;
    case Step::Closed:
      leave();
      flow = Flow::Yield;
      break;
    case Step::Finished:
      flow = Flow::Yield;
      break;
    }
  }
  m_machine.tick();
  redraw();
  ++m_frame;
}

void ContinueScene::compose(std::vector<uint32_t> &frame) const {
  systems::rasterize(output(), frame);
}

systems::Display ContinueScene::output() const {
  systems::Display display;
  display.width = WIDTH;
  display.height = HEIGHT;
  display.displayHeight = HEIGHT;
  display.border = m_session.border;
  if (!m_shown) {
    return display;
  }
  systems::Layer layer;
  layer.pixels = m_display.pixels().data();
  layer.stride = WIDTH;
  layer.sourceColumns = WIDTH;
  layer.sourceRows =
      std::min(HEIGHT, static_cast<int>(m_display.pixels().size() / WIDTH));
  layer.columns = WIDTH;
  layer.rows = HEIGHT;
  layer.mask = COLORS - 1;
  layer.palette = m_palette;
  display.layers.push_back(std::move(layer));
  return display;
}

ContinueScene::Outcome ContinueScene::outcome() const { return m_outcome; }

bool ContinueScene::isShown() const { return m_shown; }

bool ContinueScene::isContinueChosen() const { return m_continue; }

const BobLayer &ContinueScene::bobs() const { return m_bobs; }

const effects::AmigaPalette &ContinueScene::palette() const {
  return m_palette;
}

void ContinueScene::open() {
  m_screen.fill(0);
  BobLayer::paste(m_screen, m_images, QUESTION_X, QUESTION_Y, QUESTION_IMAGE);
  m_bobs.set(HAND, LEFT_X, HAND_Y, HAND_IMAGE);
  m_machine.bind(HAND, &m_bobs.object(HAND));
  m_machine.create(HAND, amal::actors::pointingHand());
  m_machine.startAll();
  m_palette = continuePalette();
  m_session.border = m_palette[0];
  m_shown = true;
  m_continue = true;
}

ContinueScene::Flow ContinueScene::choose(int16_t joystick) {
  if (joystick & JOY_LEFT) {
    m_bobs.set(HAND, LEFT_X, HAND_Y, HAND_IMAGE);
    m_continue = true;
  }
  if (joystick & JOY_RIGHT) {
    m_bobs.set(HAND, RIGHT_X, HAND_Y, HAND_IMAGE | ImageBank::FLIP_X);
    m_continue = false;
  }
  if (!(joystick & JOY_FIRE)) {
    return Flow::Yield;
  }
  m_machine.channelRegister(HAND, WAGGLE_REGISTER) = 1;
  m_resumeFrame = m_frame + MACH_WAIT;
  m_step = Step::Chosen;
  return Flow::Yield;
}

void ContinueScene::close() {
  if (m_session.version == GameVersion::V12) {
    m_host.playSample(VOICE_BANK, m_session.registers[RQ] + 1, ALL_VOICES);
  }
  m_machine.destroyAll();
  m_resumeFrame = m_frame + effects::SCREEN_CLOSE_SHOWN_VBLS;
  m_step = Step::Gone;
}

void ContinueScene::leave() {
  if (m_continue) {
    --m_session.stageReached;
    if (m_session.version == GameVersion::V12) {
      m_session.stageReached = 0;
    }
    m_session.registers[RO] = static_cast<int16_t>(m_session.stageReached);
    m_outcome = Outcome::Continue;
  } else {
    m_outcome = Outcome::NewGame;
  }
  m_step = Step::Finished;
}

void ContinueScene::redraw() {
  if (!m_shown) {
    return;
  }
  m_display = m_screen;
  m_bobs.draw(m_display, m_images);
}

} // namespace openfranko::src::engine::street
