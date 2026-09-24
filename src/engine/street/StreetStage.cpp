#include "StreetStage.h"

#include "../amal/Actors.h"

#include <algorithm>
#include <cstdlib>

namespace openfranko::src::engine::street {
namespace {

using amal::actors::amosBool;

constexpr int RA = 0;
constexpr int RB = 1;
constexpr int RC = 2;
constexpr int RD = 3;
constexpr int RE = 4;
constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RI = 8;
constexpr int RM = 12;
constexpr int RN = 13;
constexpr int RO = 14;
constexpr int RQ = 16;
constexpr int RV = 21;
constexpr int RW = 22;
constexpr int RY = 24;
constexpr int RZ = 25;

constexpr int PLAYER = 1;
constexpr int PLAYER_BLOOD = 10;
constexpr int ENEMY_BLOOD = 11;
constexpr int INDICATOR = 12;
constexpr int HIDDEN_IMAGE = 10;
constexpr int SPLAT_IMAGE = 9;
constexpr int IDLE_IMAGE = 17;

constexpr int SCREEN_SHAKE_CHANNEL = 0;
constexpr int INDICATOR_CHANNEL = 13;
constexpr int ENEMY_BLOOD_CHANNEL = 14;
constexpr int PLAYER_BLOOD_CHANNEL = 15;

constexpr int COLUMNS_PER_CHUNK = 63;
constexpr int AUTOBACK_VBLS = 3;
constexpr int GAME_OVER_WAIT = 200;
constexpr int FULL_ENERGY = 64;
constexpr int STARTING_LIVES = 3;
constexpr int EXTRA_LIFE_STEP = 40;
constexpr int STREET_MUSIC_VOLUME = 30;
constexpr int OPENING_SHOUT = 12;
constexpr int ALL_VOICES = 15;
constexpr int PRIORITY_VOICE = 1;
constexpr int BACKGROUND_VOICE = 8;
constexpr int THROWN = 30000;
constexpr int STREET_Y = 172;
constexpr int PLAYER_BLOCK_WIDTH = 48;
constexpr int PLAYER_BLOCK_HEIGHT = 78;

int16_t word(int value) { return static_cast<int16_t>(value); }

int sampleBank(int request) {
  return -2 * amosBool(request < 12) -
         4 * amosBool(request > 11 && request < 15) -
         5 * amosBool(request > 14 && request < 18) -
         6 * amosBool(request > 17 && request < 21);
}

int rebasedSample(int request) {
  return request + 11 * amosBool(request > 11) + 3 * amosBool(request > 14) +
         3 * amosBool(request > 17);
}

} // namespace

StreetStage::StreetStage(StreetHost &host, GameSession &session,
                         effects::GameOptions &options)
    : m_host(host), m_session(session), m_options(options),
      m_machine(session.registers), m_screen(SCREEN_WIDTH, SCREEN_HEIGHT),
      m_display(SCREEN_WIDTH, SCREEN_HEIGHT),
      m_screenDisplay{DISPLAY_X, DISPLAY_TOP, 0},
      m_palette(levelPalette(false)), m_panelPalette(panelPalette()) {}

void StreetStage::advance(const StreetInput &input) {
  if (m_step == Step::Finished) {
    return;
  }
  ++m_frame;
  if (input.key != SystemKey::None) {
    m_pendingKey = input.key;
  }
  m_machine.setJoystick(input.joystick);
  m_machine.tick();
  runBasic(input);
  redraw();
}

void StreetStage::compose(std::vector<uint32_t> &frame) const {
  composeFrame(frame, m_screenShown ? &m_display : nullptr, m_palette,
               m_screenDisplay, m_screenOffsetX, m_panel.get(), m_panelPalette);
}

StreetStage::Outcome StreetStage::outcome() const { return m_outcome; }

const BobLayer &StreetStage::bobs() const { return m_bobs; }

const IndexedSurface &StreetStage::screen() const { return m_screen; }

const IndexedSurface &StreetStage::display() const { return m_display; }

const StatusPanel *StreetStage::panel() const { return m_panel.get(); }

amal::Machine &StreetStage::machine() { return m_machine; }

int StreetStage::columnsWalked() const { return m_columnsWalked; }

int StreetStage::wavesSpawned() const { return m_wavesSpawned; }

bool StreetStage::isScreenShown() const { return m_screenShown; }

bool StreetStage::isFighting() const {
  return m_step == Step::Referee || m_step == Step::RefereeCorpseStamped ||
         m_step == Step::RefereeBloodStamped;
}

int16_t &StreetStage::global(int index) {
  return m_machine.globalRegister(index);
}

int16_t &StreetStage::reg(int channel, int index) {
  return m_machine.channelRegister(channel, index);
}

int StreetStage::xBob(int number) const { return m_bobs.x(number); }

int StreetStage::yBob(int number) const { return m_bobs.y(number); }

int StreetStage::iBob(int number) const { return m_bobs.image(number); }

bool StreetStage::bobCol(int number, int first, int last) {
  return m_bobs.collide(number, m_images, first, last);
}

bool StreetStage::col(int number) const { return m_bobs.collided(number); }

int StreetStage::stage() const { return m_session.registers[RO]; }

StatusPanel::Stats StreetStage::stats() const {
  const amal::Registers &registers = m_session.registers;
  return {registers[RF], registers[RO], registers[RN], registers[RG]};
}

void StreetStage::stall() { m_resumeFrame = m_frame + AUTOBACK_VBLS; }

StreetStage::Flow StreetStage::endOfPass() const {
  return m_passFrame == m_frame ? Flow::Yield : Flow::Continue;
}

void StreetStage::playRouted(int request, int voices) {
  m_host.playSample(sampleBank(request), rebasedSample(request), voices);
}

void StreetStage::newGame() {
  for (int i = 0; i <= 12; ++i) {
    global(i) = 0;
  }
  for (int i = 15; i <= 25; ++i) {
    global(i) = 0;
  }
  global(RF) = FULL_ENERGY;
  global(RG) = STARTING_LIVES;
  global(RQ) = m_options.character == effects::Character::Alex ? 1 : 0;
  m_resident.fill(0);
  m_needed.fill(0);
  m_escape = false;
}

void StreetStage::gameInit() {
  openScreens(false);
  global(RN) = 0;
}

void StreetStage::openScreens(bool shown) {
  m_palette = levelPalette(m_options.mono);
  m_panelPalette = panelPalette();
  m_screenShown = shown;
  m_screen.fill(0);
  m_panel =
      std::make_unique<StatusPanel>(m_host.loadPanelPicture(LOADING_STRIP),
                                    m_host.loadPanelPicture(PANEL_ARTWORK));
}

StreetStage::Flow StreetStage::stageInit() {
  m_pendingKey = SystemKey::None;
  m_host.stopMusic();
  m_images.clear();
  global(RO) = word(global(RO) + 1);
  m_energyShown = FULL_ENERGY;
  m_columnsWalked = 0;
  m_nextWave = 0;
  m_killsShown = 0;
  m_scrollPhase = 0;
  m_playerX = 80 - 144 * amosBool(stage() == 2);
  global(RA) = word(m_playerX);
  global(RB) = word(STREET_Y);
  m_facing = -32768 * amosBool(stage() == 2);
  global(RC) = word(m_facing);
  m_columnInChunk = COLUMNS_PER_CHUNK;
  m_chunk = 1;
  m_screenOffsetX = stage() == 2 ? 16 : 0;
  m_loading.queue([this] { m_host.loadMusic(stage() + 600); });
  return load(Step::StageMusic);
}

StreetStage::Flow StreetStage::stageMusic() {
  m_host.playMusic();
  m_host.setMusicVolume(m_options.music ? STREET_MUSIC_VOLUME : 0);
  m_loading.queue([this] { m_images.load(1, m_host.loadSpriteSet(0, 0)); });
  m_loading.queue([this] {
    m_images.load(11, m_host.loadSpriteSet(255 - 5 * global(RQ), 2));
  });
  m_loading.queue([this] { m_opening = m_host.loadPicture(stage() + 903); });
  m_loading.queue([this] { m_script = m_host.loadLevelScript(stage() + 900); });
  return load(Step::StageScreen);
}

StreetStage::Flow StreetStage::stageScreen() {
  m_screen.unpack(m_opening, 0, 0);
  m_step = Step::StageShown;
  stall();
  return Flow::Yield;
}

void StreetStage::stageShown() {
  m_bobs.set(PLAYER, m_playerX, (STREET_Y / 4) * 4, IDLE_IMAGE + m_facing);
  m_opening = Picture{};
  m_panel->showWaiting();
  m_screenShown = true;

  for (int i = 0; i <= 2; ++i) {
    m_machine.bind(4 + i * 2, &m_bobs.object(2 + i));
    m_machine.bind(5 + i * 2, &m_bobs.object(2 + i));
  }
  for (int bob = 2; bob <= 4; ++bob) {
    m_bobs.set(bob, 460, STREET_Y, 44);
  }
  for (int channel = 4; channel <= 9; ++channel) {
    m_machine.create(channel, amal::actors::idle());
  }
}

StreetStage::Flow StreetStage::load(Step next) {
  m_afterLoading = next;
  m_step = Step::Loading;
  return Flow::Continue;
}

bool StreetStage::grabPlayer() {
  const int left = m_playerX - 16;
  const int top = global(RB) - 77;
  m_block.emplace(m_screen, left, top, PLAYER_BLOCK_WIDTH, PLAYER_BLOCK_HEIGHT);
  return BobLayer::paste(m_screen, m_images, left, top, IDLE_IMAGE + m_facing);
}

StreetStage::Flow StreetStage::stopForLoading(Step next) {
  m_machine.destroyAll();
  m_playerX = xBob(PLAYER);
  global(RB) = word((global(RB) / 4) * 4);
  m_bobs.offAll();
  m_step = next;
  stall();
  return Flow::Yield;
}

void StreetStage::streetSetup() {
  m_bobs.set(PLAYER_BLOOD, 1000, 100, HIDDEN_IMAGE);
  m_bobs.set(ENEMY_BLOOD, 1000, 100, HIDDEN_IMAGE);
  m_bobs.set(INDICATOR, 242 + 164 * amosBool(stage() == 2), 32, HIDDEN_IMAGE);
  for (int channel = 1; channel <= 3; ++channel) {
    m_machine.bind(channel, &m_bobs.object(PLAYER));
  }
  m_machine.bind(PLAYER_BLOOD_CHANNEL, &m_bobs.object(PLAYER_BLOOD));
  m_machine.bind(ENEMY_BLOOD_CHANNEL, &m_bobs.object(ENEMY_BLOOD));
  m_machine.bind(INDICATOR_CHANNEL, &m_bobs.object(INDICATOR));

  m_machine.bind(SCREEN_SHAKE_CHANNEL, &m_screenDisplay);
  m_machine.create(SCREEN_SHAKE_CHANNEL, amal::actors::screenShake());
  const auto player = amal::actors::streetPlayer(stage());
  m_machine.create(1, player.locomotion);
  m_machine.create(2, player.damage);
  m_machine.create(3, player.clamp);
  m_machine.create(PLAYER_BLOOD_CHANNEL, amal::actors::playerBlood());
  m_machine.create(ENEMY_BLOOD_CHANNEL, amal::actors::enemyBlood());
  m_machine.startAll();

  m_panel->score(stats());
  for (int i = 0; i <= 2; ++i) {
    reg(5 + i * 2, 7) = word(m_energy[static_cast<std::size_t>(i + 1)]);
  }
  m_host.playSample(2, OPENING_SHOUT, ALL_VOICES);
}

StreetStage::Flow StreetStage::refereeTop() {
  m_passFrame = m_frame;
  if (reg(5, 2) == 2) {
    reg(5, 2) = 0;
    reg(5, 9) = 0;
    m_machine.start(4);
  }
  if (reg(7, 2) == 2) {
    reg(7, 2) = 0;
    reg(7, 9) = 0;
    m_machine.start(6);
  }
  if (reg(9, 2) == 2) {
    reg(9, 2) = 0;
    reg(9, 9) = 0;
    m_machine.start(8);
  }
  if (reg(2, 4) == 9) {
    reg(2, 4) = 0;
    global(RD) = 0;
    m_machine.start(1);
  }
  if (reg(2, 1) == 9) {
    reg(2, 1) = 0;
    global(RD) = 0;
    m_machine.start(1);
  }

  if (yBob(2) == yBob(3) && bobCol(2, 3, 3)) {
    reg(4, 3) = 1;
  }
  if (yBob(2) == yBob(4) && bobCol(2, 4, 4)) {
    reg(4, 3) = 1;
  }
  if (yBob(3) == yBob(4) && bobCol(3, 4, 4)) {
    reg(6, 3) = 1;
  }

  m_index = 2;
  return refereeEnemies();
}

StreetStage::Flow StreetStage::refereeEnemies() {
  for (; m_index <= 4; ++m_index) {
    const int i = m_index;
    reg(2 * i, 8) = word(
        m_host.random(2 + m_aggression[static_cast<std::size_t>(i - 1)]) + 1);
    const int p = 2 * i + 1;
    if (reg(p, 9) > 1000) {
      const int image = std::abs(reg(p, 9) - 1000 - reg(p, 1));
      int x = reg(p, 6) - 57 - 18 * amosBool(reg(p, 1) < 0);
      if (stage() == 2) {
        x = x < -40 ? 400 : std::max(16, x);
      } else {
        x = x > 280 ? 400 : std::min(200, x);
      }
      const bool stamped =
          BobLayer::paste(m_screen, m_images, x, reg(p, 7) - 17, image);
      if (stamped) {
        m_step = Step::RefereeCorpseStamped;
        stall();
        return Flow::Yield;
      }
      m_bobs.setImage(i, HIDDEN_IMAGE);
      reg(p, 9) = 0;
    }
  }
  return refereeMoves();
}

StreetStage::Flow StreetStage::refereeCorpseStamped() {
  m_bobs.setImage(m_index, HIDDEN_IMAGE);
  reg(2 * m_index + 1, 9) = 0;
  ++m_index;
  m_step = Step::Referee;
  return refereeEnemies();
}

StreetStage::Flow StreetStage::refereeMoves() {
  const auto inFront = [this](int i) {
    return (xBob(i) < xBob(PLAYER) && global(RC) != 0) ||
           (xBob(i) > xBob(PLAYER) && global(RC) == 0);
  };

  if (global(RD) == 2) {
    bobCol(PLAYER);
    for (int i = 2; i <= 4; ++i) {
      if (col(i) && reg(i * 2 + 1, 4) == 1 && yBob(PLAYER) < yBob(i) + 6 &&
          yBob(PLAYER) > yBob(i) - 6 && xBob(PLAYER) < 240 &&
          xBob(PLAYER) > 42 && global(RC) != reg(i * 2, 2) &&
          xBob(PLAYER) < xBob(i) - 25 * amosBool(global(RC) == 0) &&
          xBob(PLAYER) > xBob(i) + 25 * amosBool(global(RC) != 0)) {
        const int p = i * 2 + 1;
        m_bobs.setPosition(i,
                           xBob(PLAYER) + 24 + 48 * amosBool(global(RC) == 0),
                           yBob(PLAYER) - 4);
        m_machine.freeze(i * 2);
        m_machine.freeze(1);
        reg(p, 1) = reg(i * 2, 2);
        reg(p, 4) = 2;
        reg(2, 4) = 1;
        break;
      }
    }
  }

  if (global(RD) == 1 || global(RD) == 2) {
    bobCol(PLAYER);
    for (int i = 2; i <= 4; ++i) {
      if (col(i) && inFront(i) && yBob(i) == yBob(PLAYER)) {
        const int p = i * 2 + 1;
        m_machine.freeze(i * 2);
        reg(p, 1) = word(0x8000 - global(RC));
        reg(p, 2) = 1;
        reg(p, 0) = global(RD);
        break;
      }
    }
  }

  if (global(RD) == 3) {
    bobCol(PLAYER);
    for (int i = 2; i <= 4; ++i) {
      if (col(i) && yBob(i) == yBob(PLAYER)) {
        const int p = i * 2 + 1;
        m_machine.freeze(i * 2);
        reg(i * 2, 9) = 0;
        reg(p, 1) = reg(i * 2, 2);
        reg(p, 2) = 1;
        reg(p, 0) = 3;
      }
    }
  }

  if (global(RD) == 4) {
    bobCol(PLAYER);
    for (int i = 2; i <= 4; ++i) {
      if (col(i) && global(RB) == yBob(i) && reg(i * 2 + 1, 2) != 1 &&
          reg(i * 2, 3) == 0 && inFront(i)) {
        const int p = i * 2 + 1;
        m_machine.freeze(i * 2);
        reg(p, 1) = word(0x8000 - global(RC));
        reg(p, 3) = word(16 + 32 * amosBool(reg(p, 1) == 0));
        reg(p, 2) = 1;
        reg(p, 0) = 4;
      }
    }
  }

  if (global(RD) == 5) {
    bobCol(PLAYER);
    for (int i = 2; i <= 4; ++i) {
      if (col(i) && global(RB) == yBob(i) && reg(i * 2 + 1, 2) != 1) {
        const int p = i * 2 + 1;
        m_machine.freeze(i * 2);
        reg(p, 1) = reg(i * 2, 2);
        reg(p, 3) = word(16 + 32 * amosBool(reg(p, 1) == 0));
        reg(p, 2) = 1;
        reg(p, 0) = 5;
      }
    }
  }

  if (global(RD) == 6) {
    bobCol(PLAYER);
    for (int i = 2; i <= 4; ++i) {
      if (reg(i * 2 + 1, 5) == 0 && global(RC) != reg(i * 2, 2) && col(i) &&
          reg(2, 5) == 0 && yBob(PLAYER) == yBob(i) && inFront(i)) {
        const int p = i * 2 + 1;
        m_machine.freeze(i * 2);
        m_machine.freeze(1);
        reg(p, 1) = word(0x8000 - global(RC));
        m_bobs.setX(i, xBob(PLAYER) + 40 + 80 * amosBool(global(RC) != 0));
        reg(p, 6) = word(16 + 32 * amosBool(global(RC) == 0));
        reg(p, 2) = 1;
        reg(p, 3) = word(16 + 32 * amosBool(reg(p, 1) == 0));
        reg(2, 5) = 1;
        reg(p, 0) = 6;
        break;
      }
    }
  }

  for (int i = 2; i <= 4; ++i) {
    if (bobCol(i) && col(PLAYER) && global(RD) == 0 &&
        yBob(PLAYER) == yBob(i) && reg(2, 1) == 0 && reg(i * 2, 9) > 0 &&
        reg(i * 2, 9) < 4 && reg(i * 2 + 1, 0) == 0) {
      if ((reg(i * 2, 2) == 0 && xBob(PLAYER) > xBob(i)) ||
          (reg(i * 2, 2) != 0 && xBob(PLAYER) < xBob(i))) {
        m_machine.freeze(1);
        global(RV) = 0;
        reg(2, 5) = 0;
        reg(2, 9) = 50;
        reg(5, 9) = 50;
        reg(7, 9) = 50;
        reg(9, 9) = 50;
        m_bobs.setX(i, xBob(i) + 8 + 16 * amosBool(reg(2 * i, 2) == 0));
        global(RC) = word(0x8000 - reg(2 * i, 2));
        reg(2, 2) = word(32 + 64 * amosBool(global(RC) == 0));
        reg(2, 1) = reg(i * 2, 9);
      }
    }
  }

  if (global(RW) != 0) {
    const int request = global(RW);
    global(RW) = 0;
    playRouted(request, PRIORITY_VOICE);
  } else if (global(RE) != 0) {
    const int request = global(RE);
    global(RE) = 0;
    playRouted(request, BACKGROUND_VOICE);
  }

  for (int i = 2; i <= 4; ++i) {
    if (reg(i * 2 + 1, 3) != THROWN) {
      continue;
    }
    const auto sameDepth = [this, i]() {
      return yBob(3) > yBob(i) - 24 && yBob(3) < yBob(i) + 24;
    };
    const auto knockDown = [this, i](int channel) {
      m_machine.freeze(channel - 1);
      reg(channel, 1) = word(0x8000 - reg(i * 2 + 1, 1));
      reg(channel, 3) = word(16 + 32 * amosBool(reg(channel, 1) == 0));
      reg(channel, 2) = 1;
      reg(channel, 0) = 4;
      reg(i * 2 + 1, 3) = 0;
    };
    if ((i == 2 || i == 4) && bobCol(i) && col(3) && sameDepth()) {
      knockDown(7);
    }
    if ((i == 3 || i == 2) && bobCol(i) && col(4) && sameDepth()) {
      knockDown(9);
    }
    if ((i == 3 || i == 4) && bobCol(i) && col(2) && sameDepth()) {
      knockDown(5);
    }
  }

  if (m_escape || global(RG) == -2) {
    gameOver();
    return Flow::Continue;
  }
  if (global(RI) < 0) {
    m_step = Step::AdvanceWait;
    return Flow::Continue;
  }
  if (global(RI) == 0) {
    global(RI) = -1;
  }

  m_index = PLAYER_BLOOD;
  return refereeBlood();
}

StreetStage::Flow StreetStage::refereeBlood() {
  for (; m_index <= ENEMY_BLOOD; ++m_index) {
    const int i = m_index;
    if (iBob(i) != SPLAT_IMAGE) {
      continue;
    }
    const int x = xBob(i) + m_host.random(8) - m_host.random(8);
    const int y = yBob(i) + m_host.random(8) - m_host.random(8);
    const int image = m_host.random(7) + 2;
    if (BobLayer::paste(m_screen, m_images, x, y, image)) {
      m_step = Step::RefereeBloodStamped;
      stall();
      return Flow::Yield;
    }
    m_bobs.setImage(i, HIDDEN_IMAGE);
  }
  return refereeTail();
}

StreetStage::Flow StreetStage::refereeBloodStamped() {
  m_bobs.setImage(m_index, HIDDEN_IMAGE);
  ++m_index;
  m_step = Step::Referee;
  return refereeBlood();
}

StreetStage::Flow StreetStage::refereeTail() {
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
  sys();
  m_step = Step::Referee;
  return endOfPass();
}

StreetStage::Flow StreetStage::advanceWait() {
  if (global(RZ) != 0 || global(RY) != 0 || global(RM) != 0) {
    return Flow::Yield;
  }
  advanceSetup();
  m_step = Step::Advance;
  return Flow::Continue;
}

void StreetStage::advanceSetup() {
  for (int channel = 4; channel <= 9; ++channel) {
    m_machine.destroy(channel);
  }
  m_machine.create(INDICATOR_CHANNEL, amal::actors::indicatorArrow(m_facing));
  m_machine.start(INDICATOR_CHANNEL);
  m_scrollPhase = 1;
  m_bobs.set(PLAYER_BLOOD, 120, 24, HIDDEN_IMAGE);
  m_bobs.set(ENEMY_BLOOD, 120, 24, HIDDEN_IMAGE);
}

StreetStage::Flow StreetStage::advanceTop(const StreetInput &input) {
  m_passFrame = m_frame;
  const bool waveDue =
      m_nextWave < static_cast<int>(m_script.waves.size()) &&
      m_script.waves[static_cast<std::size_t>(m_nextWave)].trigger ==
          m_columnsWalked;
  if (waveDue && m_scrollPhase == 1) {
    return stopForLoading(Step::SpawnFlushed);
  }
  if (global(RE) != 0) {
    m_host.playSample(2, global(RE), PRIORITY_VOICE);
    global(RE) = 0;
  }
  if (m_columnInChunk == COLUMNS_PER_CHUNK) {
    m_machine.freezeAll();
    m_bobs.setPosition(PLAYER, xBob(PLAYER), (global(RB) / 4) * 4);
    m_bobs.setImage(PLAYER, IDLE_IMAGE + m_facing);
    m_loading.queue([this] {
      m_columns = m_host.loadScenery(300 + stage() * 10 + m_chunk);
    });
    return load(Step::AdvanceChunkLoaded);
  }
  return advanceWalk(input);
}

StreetStage::Flow StreetStage::advanceChunkLoaded(const StreetInput &input) {
  m_panel->score(stats());
  ++m_chunk;
  m_columnInChunk = 0;
  m_machine.startAll();
  return advanceWalk(input);
}

StreetStage::Flow StreetStage::advanceWalk(const StreetInput &input) {
  const int joystick = input.joystick;
  bool walking = false;
  int bias = 0;
  if (stage() == 2) {
    walking = xBob(PLAYER) < 164 && joystick < 16 && (joystick & 4) &&
              iBob(PLAYER) < 17 && global(RD) == 0;
    bias = 6 - 2 * amosBool(xBob(PLAYER) < 152);
  } else {
    walking = xBob(PLAYER) > 152 && joystick < 16 && (joystick & 8) &&
              iBob(PLAYER) < 17 && global(RD) == 0;
    bias = 6 - 2 * amosBool(xBob(PLAYER) > 164);
  }
  if (!walking) {
    return advanceTail();
  }
  reg(1, 1) = word(bias);
  m_scrollPhase = m_scrollPhase + 1 > 1 ? 0 : m_scrollPhase + 1;
  if (m_scrollPhase == 0) {
    m_screen.unpack(m_columns.at(static_cast<std::size_t>(m_columnInChunk)),
                    stage() == 2 ? 0 : 304, 0);
    ++m_columnInChunk;
    ++m_columnsWalked;
    m_step = Step::AdvanceScroll;
    stall();
    return Flow::Yield;
  }
  return advanceScroll();
}

StreetStage::Flow StreetStage::advanceScroll() {
  scrollStep();
  m_step = Step::AdvanceWalked;
  stall();
  return Flow::Yield;
}

StreetStage::Flow StreetStage::advanceWalked() {
  reg(1, 1) = 0;
  return advanceTail();
}

StreetStage::Flow StreetStage::advanceTail() {
  if (m_columnsWalked > 10 && m_columnsWalked == m_script.length - 1) {
    scrollStep();
    m_step = Step::AdvanceLeave;
    stall();
    return Flow::Yield;
  }
  sys();
  if (m_escape) {
    gameOver();
    return Flow::Continue;
  }
  m_step = Step::Advance;
  return endOfPass();
}

StreetStage::Flow StreetStage::advanceLeaveFlushed() {
  m_step = Step::AdvanceLeavePasted;
  if (grabPlayer()) {
    stall();
    return Flow::Yield;
  }
  return Flow::Continue;
}

StreetStage::Flow StreetStage::advanceLeavePasted() {
  m_session.streetExit.emplace(
      StreetExit{m_screen, *m_block, m_playerX, m_energyShown, m_killsShown});
  m_block.reset();
  m_outcome = Outcome::LevelFinished;
  m_step = Step::Finished;
  return Flow::Yield;
}

StreetStage::Flow StreetStage::spawnFlushed() {
  m_step = Step::SpawnPasted;
  if (grabPlayer()) {
    stall();
    return Flow::Yield;
  }
  return Flow::Continue;
}

StreetStage::Flow StreetStage::spawnPasted() {
  m_panel->showWaiting();
  const Wave &wave = m_script.waves[static_cast<std::size_t>(m_nextWave)];
  global(RI) = 3;

  for (int i = 1; i <= 3; ++i) {
    const int resident = m_resident[static_cast<std::size_t>(i)];
    m_needed[static_cast<std::size_t>(i)] =
        amosBool(wave.slots[0].spriteSet == resident ||
                 wave.slots[1].spriteSet == resident ||
                 wave.slots[2].spriteSet == resident);
  }
  int slot = 0;
  for (const EnemySlot &enemy : wave.slots) {
    bool missing = false;
    for (int i = 1; i <= 3; ++i) {
      if (enemy.spriteSet != m_resident[static_cast<std::size_t>(i)] &&
          enemy.spriteSet != EnemySlot::EMPTY) {
        missing = true;
      } else {
        missing = false;
        break;
      }
    }
    if (missing) {
      int base = 0;
      if (m_needed[3] == 0) {
        slot = 3;
        base = 94;
      }
      if (m_needed[2] == 0) {
        slot = 2;
        base = 69;
      }
      if (m_needed[1] == 0) {
        slot = 1;
        base = 44;
      }
      const int spriteSet = enemy.spriteSet;
      const int bank = 4 + slot - 1;
      m_loading.queue([this, base, spriteSet, bank] {
        m_images.load(base, m_host.loadSpriteSet(spriteSet, bank));
      });
      m_resident[static_cast<std::size_t>(slot)] = enemy.spriteSet;
      m_needed[static_cast<std::size_t>(slot)] = -1;
    }
  }
  return load(Step::SpawnLoaded);
}

void StreetStage::spawnLoaded() {
  const Wave &wave = m_script.waves[static_cast<std::size_t>(m_nextWave)];
  for (int j = 2; j <= 4; ++j) {
    const EnemySlot &enemy = wave.slots[static_cast<std::size_t>(j - 2)];
    m_machine.bind(j * 2, &m_bobs.object(j));
    m_machine.bind(j * 2 + 1, &m_bobs.object(j));
    if (enemy.spriteSet == EnemySlot::EMPTY) {
      m_bobs.set(j, 1000, 300, HIDDEN_IMAGE);
      m_machine.create(j * 2, amal::actors::idle());
      m_machine.create(j * 2 + 1, amal::actors::idle());
      global(RI) = word(global(RI) - 1);
      continue;
    }
    m_energy[static_cast<std::size_t>(j - 1)] = enemy.energy;
    m_aggression[static_cast<std::size_t>(j - 1)] = enemy.aggression;
    m_bobs.set(j, enemy.x, enemy.y, HIDDEN_IMAGE);
    const int base = 0 - 25 * amosBool(m_resident[2] == enemy.spriteSet) -
                     50 * amosBool(m_resident[3] == enemy.spriteSet);
    const auto programs = amal::actors::enemy(base, enemy.type);
    m_machine.create(j * 2, programs.walk);
    m_machine.create(j * 2 + 1, programs.damage);
  }

  ++m_nextWave;
  ++m_wavesSpawned;
  m_bobs.set(PLAYER, m_playerX, (global(RB) / 4) * 4, IDLE_IMAGE + m_facing);
  m_block->put(m_screen);
  m_block.reset();
}

void StreetStage::scrollStep() {
  const int dx = stage() == 2 ? 8 : -8;
  m_screen.copy(m_screen, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, dx, 0);
}

void StreetStage::gameOver() {
  m_session.stageReached = global(RO);
  global(RO) = -1;
  if (m_escape) {
    global(RN) = 0;
    m_outcome = Outcome::Quit;
    m_step = Step::Finished;
    return;
  }
  m_resumeFrame = m_frame + GAME_OVER_WAIT;
  m_step = Step::GameOverWait;
}

void StreetStage::sys() {
  const SystemKey key = m_pendingKey;
  m_pendingKey = SystemKey::None;
  switch (key) {
  case SystemKey::MusicOff:
    m_host.setMusicVolume(0);
    m_options.music = false;
    break;
  case SystemKey::MusicOn:
    m_options.music = true;
    m_host.setMusicVolume(STREET_MUSIC_VOLUME);
    break;
  case SystemKey::Escape:
    global(RN) = 0;
    m_escape = true;
    m_machine.freezeAll();
    break;
  case SystemKey::None:
    break;
  }
}

void StreetStage::runBasic(const StreetInput &input) {
  Flow flow = Flow::Continue;
  while (flow == Flow::Continue && m_frame >= m_resumeFrame) {
    switch (m_step) {
    case Step::Start:
      if (m_session.fromBonusDrive) {
        m_session.fromBonusDrive = false;
        openScreens(true);
      } else {
        newGame();
        gameInit();
      }
      flow = stageInit();
      break;
    case Step::StageMusic:
      flow = stageMusic();
      break;
    case Step::StageScreen:
      flow = stageScreen();
      break;
    case Step::StageShown:
      stageShown();
      streetSetup();
      m_step = Step::Referee;
      break;
    case Step::Loading:
      if (m_loading.advance(m_panel.get())) {
        m_step = m_afterLoading;
      } else {
        flow = Flow::Yield;
      }
      break;
    case Step::Referee:
      flow = refereeTop();
      break;
    case Step::RefereeCorpseStamped:
      flow = refereeCorpseStamped();
      break;
    case Step::RefereeBloodStamped:
      flow = refereeBloodStamped();
      break;
    case Step::AdvanceWait:
      flow = advanceWait();
      break;
    case Step::Advance:
      flow = advanceTop(input);
      break;
    case Step::AdvanceChunkLoaded:
      flow = advanceChunkLoaded(input);
      break;
    case Step::AdvanceScroll:
      flow = advanceScroll();
      break;
    case Step::AdvanceWalked:
      flow = advanceWalked();
      break;
    case Step::SpawnFlushed:
      flow = spawnFlushed();
      break;
    case Step::SpawnPasted:
      flow = spawnPasted();
      break;
    case Step::SpawnLoaded:
      spawnLoaded();
      streetSetup();
      m_step = Step::Referee;
      break;
    case Step::AdvanceLeave:
      flow = stopForLoading(Step::AdvanceLeaveFlushed);
      break;
    case Step::AdvanceLeaveFlushed:
      flow = advanceLeaveFlushed();
      break;
    case Step::AdvanceLeavePasted:
      flow = advanceLeavePasted();
      break;
    case Step::GameOverWait:
      m_outcome = Outcome::GameOver;
      m_step = Step::Finished;
      flow = Flow::Yield;
      break;
    case Step::Finished:
      flow = Flow::Yield;
      break;
    }
  }
}

void StreetStage::redraw() {
  m_display = m_screen;
  m_bobs.draw(m_display, m_images);
}

} // namespace openfranko::src::engine::street
