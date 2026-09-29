#include "Stage.h"

#include "../../AmigaDisplay.h"

#include <utility>

namespace openfranko::src::engine::street::scenes {
namespace {

constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RI = 8;
constexpr int RN = 13;
constexpr int RO = 14;

constexpr int GAME_OVER_WAIT = 200;
constexpr int EXTRA_LIFE_STEP = 40;
constexpr int MUSIC_VOLUME = 30;
constexpr int PLAYER_CHANNEL = 1;
constexpr int16_t CHEAT_LIVES = 12;

} // namespace

Stage::Stage(StreetHost &host, session::GameSession &session,
             GameOptions &options)
    : m_host(host), m_session(session), m_options(options),
      m_machine(session.registers), m_screen(SCREEN_WIDTH, SCREEN_HEIGHT),
      m_buffer(m_screen),
      m_screenDisplay{
          ui::DISPLAY_X,
          static_cast<int16_t>(ui::playDisplayY(ui::stageLayout(options))), 0},
      m_palette(ui::levelPalette(options.mono)),
      m_panelPalette(ui::panelPalette()) {
  m_copper.reset(registers());
  m_session.border = ui::STAGE_BORDER;
}

void Stage::advance(const StreetInput &input) {
  if (m_outcome != Outcome::Playing) {
    return;
  }
  ++m_frame;
  if (input.key != session::SystemKey::None) {
    m_session.keyLatch = input.key;
  }
  m_mouseButton = input.mouseButton;
  m_buffer.vbl();
  m_copper.vbl(m_options.ntsc);
  m_machine.setJoystick(input.joystick);
  m_machine.tick();
  if (m_buffer.isAutobacking()) {
    m_buffer.autobackStep(m_bobs, m_images);
  } else if (!m_hold.holdsAtStart(m_frame)) {
    test();
  }
  runBasic(input);
  if (!m_buffer.isAutobacking() && !m_hold.holdsAtEnd(m_frame)) {
    test();
  }
}

void Stage::compose(std::vector<uint32_t> &frame) const {
  systems::graphics::rasterize(output(), frame);
}

systems::graphics::Display Stage::output() const {
  const ui::StageCopper &live = m_copper.live();
  return ui::stageOutput(live.screenShown ? &m_buffer.shown() : nullptr,
                         m_palette, live.screenDisplay, m_screenOffsetX,
                         m_panelShown ? m_panel.get() : nullptr,
                         m_copper.panelY(m_options.tallScreen), m_panelPalette,
                         m_copper.window(m_options.tallScreen));
}

Stage::Outcome Stage::outcome() const { return m_outcome; }

const core::BobLayer &Stage::bobs() const { return m_bobs; }

const core::IndexedSurface &Stage::screen() const { return m_screen; }

const core::IndexedSurface &Stage::display() const { return m_buffer.shown(); }

const ui::StatusPanel *Stage::panel() const { return m_panel.get(); }

amal::Machine &Stage::machine() { return m_machine; }

bool Stage::isScreenShown() const { return m_copper.live().screenShown; }

bool Stage::isPanelShown() const { return m_panelShown; }

int16_t Stage::word(int value) { return static_cast<int16_t>(value); }

int16_t &Stage::global(int index) { return m_machine.globalRegister(index); }

int16_t &Stage::reg(int channel, int index) {
  return m_machine.channelRegister(channel, index);
}

int Stage::xBob(int number) const { return m_bobs.x(number); }

int Stage::yBob(int number) const { return m_bobs.y(number); }

int Stage::iBob(int number) const { return m_bobs.image(number); }

bool Stage::col(int number) const { return m_bobs.collided(number); }

int Stage::stage() const { return m_session.registers[RO]; }

ui::StatusPanel::Stats Stage::stats() const {
  const amal::Registers &registers = m_session.registers;
  return {registers[RF], registers[RO], registers[RN], registers[RG]};
}

void Stage::openPanel() {
  m_panel = std::make_unique<ui::StatusPanel>(
      m_host.loadPanelPicture(StreetHost::LOADING_STRIP),
      m_host.loadPanelPicture(StreetHost::PANEL_ARTWORK), m_session.version);
}

void Stage::playMusic() {
  m_host.playMusic();
  m_host.setMusicVolume(m_options.music ? MUSIC_VOLUME : 0);
}

void Stage::updatePanel() {
  if (m_energyShown != global(RF)) {
    if (m_energyShown > global(RF)) {
      m_panel->loseEnergy(global(RF));
    } else {
      m_panel->score(stats());
    }
    m_energyShown = global(RF);
  }
  if (m_killsShown != global(RN)) {
    m_panel->drawKills(global(RN));
    m_killsShown = global(RN);
  }
  if (m_session.extraLifeKills == global(RN)) {
    global(RF) = FULL_ENERGY;
    global(RG) = word(global(RG) + 1);
    m_panel->score(stats());
    m_session.extraLifeKills += EXTRA_LIFE_STEP;
  }
}

void Stage::stall() { m_resumeFrame = m_frame + AUTOBACK_VBLS; }

void Stage::autoback(core::DoubleBuffer::Op op) {
  op(m_screen);
  m_buffer.autoback(std::move(op));
  stall();
}

bool Stage::pasteStalled(int x, int y, int image) {
  if (!core::BobLayer::paste(m_screen, m_images, x, y, image)) {
    return false;
  }
  m_buffer.autoback([this, x, y, image](core::IndexedSurface &surface) {
    core::BobLayer::paste(surface, m_images, x, y, image);
  });
  stall();
  return true;
}

void Stage::putBlock(const core::ScreenBlock &block) {
  block.put(m_screen);
  block.put(m_buffer.logic());
  m_buffer.swap();
  block.put(m_buffer.logic());
  m_buffer.swap();
}

void Stage::scrollStep() {
  const int dx = stage() == 2 ? 8 : -8;
  autoback([dx](core::IndexedSurface &surface) {
    surface.copy(surface, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, dx, 0);
  });
}

Stage::Flow Stage::endOfPass() const {
  return m_passFrame == m_frame ? Flow::Yield : Flow::Continue;
}

Stage::Flow Stage::hold(int frames) {
  m_hold.start(m_frame, frames);
  m_resumeFrame = m_frame + frames;
  return Flow::Yield;
}

void Stage::test() {
  if (m_buffer.test(m_bobs, m_images)) {
    m_copper.rebuild(registers());
  }
}

ui::StageCopper Stage::registers() const {
  return {m_screenShown, m_screenDisplay, m_options.ntsc};
}

void Stage::hideScreen() {
  m_screenShown = false;
  m_copper.hide();
}

void Stage::gameOver() {
  m_session.stageReached = global(RO);
  global(RO) = -1;
  if (quitsToHighScores()) {
    global(RN) = 0;
    closePlayScreen();
    return;
  }
  m_resumeFrame = m_frame + GAME_OVER_WAIT;
  m_closing = Closing::Wait;
}

Stage::Flow Stage::advanceGameOver() {
  switch (m_closing) {
  case Closing::Wait:
    if (m_session.version != GameVersion::V12) {
      return closePlayScreen();
    }
    m_bobs.offAll();
    autoback([](core::IndexedSurface &surface) { surface.fill(0); });
    m_closing = Closing::Cleared;
    return Flow::Yield;
  case Closing::Cleared:
    return closePlayScreen();
  case Closing::ScreenGone:
    hideScreen();
    m_closing = Closing::PanelClose;
    return closeWait(SCREEN_CLOSE_HIDDEN_VBLS);
  case Closing::PanelClose:
    m_closing = Closing::PanelGone;
    return closeWait(SCREEN_CLOSE_SHOWN_VBLS);
  case Closing::PanelGone:
    m_panelShown = false;
    m_closing = Closing::Closed;
    return closeWait(SCREEN_CLOSE_HIDDEN_VBLS);
  case Closing::Closed:
    m_outcome = quitsToHighScores() ? Outcome::Quit : Outcome::GameOver;
    break;
  }
  return Flow::Yield;
}

Stage::Flow Stage::closePlayScreen() {
  m_closing = Closing::ScreenGone;
  return closeWait(SCREEN_CLOSE_SHOWN_VBLS);
}

Stage::Flow Stage::closeWait(int frames) {
  if (m_holdsWhileClosing) {
    return hold(frames);
  }
  m_resumeFrame = m_frame + frames;
  return Flow::Yield;
}

bool Stage::quitsToHighScores() const {
  return m_escape && m_session.version == GameVersion::V10;
}

void Stage::sys() {
  const session::SystemKey key =
      std::exchange(m_session.keyLatch, session::SystemKey::None);
  switch (key) {
  case session::SystemKey::MusicOff:
    if (m_musicLoaded) {
      m_host.setMusicVolume(0);
      m_options.music = false;
    }
    break;
  case session::SystemKey::MusicOn:
    if (m_musicLoaded) {
      m_options.music = true;
      m_host.setMusicVolume(MUSIC_VOLUME);
    }
    break;
  case session::SystemKey::Pal:
  case session::SystemKey::Ntsc:
    ui::switchStandard(m_options, m_screenDisplay,
                       key == session::SystemKey::Ntsc);
    break;
  case session::SystemKey::Lives:
    global(RG) = CHEAT_LIVES;
    m_panel->score(stats());
    break;
  case session::SystemKey::Escape:
    global(RN) = 0;
    m_escape = true;
    m_machine.freezeAll();
    break;
  case session::SystemKey::None:
  case session::SystemKey::Other:
    break;
  }
  if (m_mouseButton && global(RI) > 0) {
    global(RN) = word(global(RN) + global(RI));
    global(RI) = 0;
    m_machine.start(PLAYER_CHANNEL);
  }
}

} // namespace openfranko::src::engine::street::scenes
