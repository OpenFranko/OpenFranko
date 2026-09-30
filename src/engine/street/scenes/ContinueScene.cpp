#include "ContinueScene.h"

#include "../../../systems/audio/Mixer.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../AmigaDisplay.h"
#include "../../assets/Assets.h"
#include "../actors/compiled/CompiledActors.h"
#include "../ui/ScreenOutput.h"

#include <cstddef>

namespace openfranko::src::engine::street::scenes {
namespace {

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

constexpr std::size_t COLORS = 32;

effects::color::AmigaPalette continuePalette() {
  effects::color::AmigaPalette palette(COLORS, effects::color::BLACK);
  palette[0] = 0x707;
  palette[18] = 0xAAA;
  palette[24] = 0xDDD;
  palette[29] = 0x769;
  palette[30] = 0xB95;
  palette[31] = 0xFC0;
  return palette;
}

} // namespace

ContinueScene::ContinueScene(StreetHost &host, session::GameSession &session)
    : m_host(host), m_session(session), m_machine(session.registers),
      m_screen(SCREEN_WIDTH, SCREEN_HEIGHT),
      m_display(SCREEN_WIDTH, SCREEN_HEIGHT),
      m_palette(COLORS, effects::color::BLACK) {
  const bool voices = session.version == GameVersion::V12;
  m_images.load(
      core::ImageBank::FIRST_IMAGE,
      host.loadSpriteSet(assets::LETTER_SET, voices ? VOICE_BANK : 0));
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
      m_resumeFrame = m_frame + SCREEN_CLOSE_HIDDEN_VBLS;
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
  systems::graphics::rasterize(output(), frame);
}

systems::graphics::Display ContinueScene::output() const {
  return ui::screenOutput(m_display, m_shown, m_palette, m_session.border);
}

ContinueScene::Outcome ContinueScene::outcome() const { return m_outcome; }

bool ContinueScene::isShown() const { return m_shown; }

bool ContinueScene::isContinueChosen() const { return m_continue; }

const core::BobLayer &ContinueScene::bobs() const { return m_bobs; }

const effects::color::AmigaPalette &ContinueScene::palette() const {
  return m_palette;
}

void ContinueScene::open() {
  m_screen.fill(0);
  core::BobLayer::paste(m_screen, m_images, QUESTION_X, QUESTION_Y,
                        QUESTION_IMAGE);
  m_bobs.set(HAND, LEFT_X, HAND_Y, HAND_IMAGE);
  m_machine.bind(HAND, &m_bobs.object(HAND));
  m_machine.create(HAND, actors::compiled::pointingHand());
  m_machine.startAll();
  m_palette = continuePalette();
  m_session.border = m_palette[0];
  m_shown = true;
  m_continue = true;
}

ContinueScene::Flow ContinueScene::choose(int16_t joystick) {
  if (joystick & systems::input::JOY_LEFT) {
    m_bobs.set(HAND, LEFT_X, HAND_Y, HAND_IMAGE);
    m_continue = true;
  }
  if (joystick & systems::input::JOY_RIGHT) {
    m_bobs.set(HAND, RIGHT_X, HAND_Y, HAND_IMAGE | core::ImageBank::FLIP_X);
    m_continue = false;
  }
  if (!(joystick & systems::input::JOY_FIRE)) {
    return Flow::Yield;
  }
  m_machine.channelRegister(HAND, WAGGLE_REGISTER) = 1;
  m_resumeFrame = m_frame + MACH_WAIT;
  m_step = Step::Chosen;
  return Flow::Yield;
}

void ContinueScene::close() {
  if (m_session.version == GameVersion::V12) {
    m_host.playSample(VOICE_BANK, m_session.registers[amal::RQ] + 1,
                      systems::audio::Mixer::ALL_VOICES);
  }
  m_machine.destroyAll();
  m_resumeFrame = m_frame + SCREEN_CLOSE_SHOWN_VBLS;
  m_step = Step::Gone;
}

void ContinueScene::leave() {
  if (m_continue) {
    --m_session.stageReached;
    if (m_session.version == GameVersion::V12) {
      m_session.stageReached = 0;
    }
    m_session.registers[amal::RO] =
        static_cast<int16_t>(m_session.stageReached);
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

} // namespace openfranko::src::engine::street::scenes
