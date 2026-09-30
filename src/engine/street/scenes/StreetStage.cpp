#include "StreetStage.h"

#include "../../../systems/audio/Mixer.h"
#include "../../AmigaDisplay.h"
#include "../actors/Actors.h"

#include <algorithm>
#include <cstdlib>

namespace openfranko::src::engine::street::scenes {
namespace {

constexpr int PLAYER_BLOOD = 10;
constexpr int ENEMY_BLOOD = 11;

constexpr int COLUMNS_PER_CHUNK = 63;
constexpr int SHORT_LEVEL_LENGTH = 32;
constexpr int OPENING_SHOUT = 12;
constexpr int BACKGROUND_VOICE = 8;
constexpr int THROWN = 30000;
constexpr int PLAYER_BLOCK_WIDTH = 48;
constexpr int PLAYER_BLOCK_HEIGHT = 78;

int sampleBank(int request) {
  return -2 * actors::amosBool(request < 12) -
         4 * actors::amosBool(request > 11 && request < 15) -
         5 * actors::amosBool(request > 14 && request < 18) -
         6 * actors::amosBool(request > 17 && request < 21);
}

int rebasedSample(int request) {
  return request + 11 * actors::amosBool(request > 11) +
         3 * actors::amosBool(request > 14) +
         3 * actors::amosBool(request > 17);
}

} // namespace

StreetStage::StreetStage(StreetHost &host, session::GameSession &session,
                         GameOptions &options)
    : Stage(host, session, options) {
  hideScreen();
}

int StreetStage::columnsWalked() const { return m_columnsWalked; }

int StreetStage::wavesSpawned() const { return m_wavesSpawned; }

bool StreetStage::isFighting() const {
  return m_step == Step::Referee || m_step == Step::RefereeCorpseStamped ||
         m_step == Step::RefereeBloodStamped;
}

bool StreetStage::bobCol(int number, int first, int last) {
  return m_bobs.collide(number, m_images, first, last);
}

void StreetStage::playRouted(int request, int voices) {
  m_host.playSample(sampleBank(request), rebasedSample(request), voices);
}

void StreetStage::newGame() {
  global(amal::RQ) = m_options.character == Character::Alex ? 1 : 0;
  m_resident.fill(0);
  m_needed.fill(0);
  m_escape = false;
}

void StreetStage::gameInit() {
  openScreens(false);
  m_panelShown = false;
  global(amal::RN) = 0;
}

void StreetStage::openScreens(bool shown) {
  m_palette = ui::levelPalette(m_options.mono);
  m_panelPalette = ui::panelPalette();
  m_screenShown = shown;
  m_copper.reset(registers());
  m_screen.fill(0);
  m_buffer = core::DoubleBuffer(m_screen);
  openPanel();
}

StreetStage::Flow StreetStage::stageInit() {
  m_session.keyLatch = session::SystemKey::None;
  m_host.stopMusic();
  m_images.clear();
  global(amal::RO) = word(global(amal::RO) + 1);
  m_energyShown = session::FULL_ENERGY;
  m_columnsWalked = 0;
  m_nextWave = 0;
  m_killsShown = 0;
  m_scrollPhase = 0;
  m_playerX = 80 - 144 * actors::amosBool(stage() == 2);
  global(amal::RA) = word(m_playerX);
  global(amal::RB) = word(STREET_Y);
  m_facing = -32768 * actors::amosBool(stage() == 2);
  global(amal::RC) = word(m_facing);
  m_columnInChunk = COLUMNS_PER_CHUNK;
  m_chunk = 1;
  m_screenOffsetX = stage() == 2 ? 16 : 0;
  m_loading.queue([this] { m_host.loadMusic(stage() + 600); });
  return load(Step::StageMusic);
}

StreetStage::Flow StreetStage::stageMusic() {
  playMusic();
  m_loading.queue([this] { m_images.load(1, m_host.loadSpriteSet(0, 0)); });
  m_loading.queue([this] {
    m_images.load(11, m_host.loadSpriteSet(255 - 5 * global(amal::RQ), 2));
  });
  m_loading.queue([this] { m_opening = m_host.loadPicture(stage() + 903); });
  m_loading.queue([this] {
    m_script = m_host.loadLevelScript(stage() + 900);
    if (m_session.shortLevels) {
      m_script.length = SHORT_LEVEL_LENGTH;
    }
  });
  return load(Step::StageScreen);
}

StreetStage::Flow StreetStage::stageScreen() {
  m_step = Step::StageShown;
  autoback([opening = m_opening](core::IndexedSurface &surface) {
    surface.unpack(opening, 0, 0);
  });
  return Flow::Yield;
}

void StreetStage::stageShown() {
  m_bobs.set(PLAYER, m_playerX, (STREET_Y / 4) * 4, IDLE_IMAGE + m_facing);
  m_opening = core::Picture{};
  test();
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
    m_machine.create(channel, actors::idle(m_session.version));
  }
}

StreetStage::Flow StreetStage::load(Step next) {
  m_afterLoading = next;
  m_step = Step::Loading;
  return Flow::Continue;
}

bool StreetStage::grabPlayer() {
  const int left = m_playerX - 16;
  const int top = global(amal::RB) - 77;
  m_block.emplace(m_buffer.logic(), left, top, PLAYER_BLOCK_WIDTH,
                  PLAYER_BLOCK_HEIGHT);
  return pasteStalled(left, top, IDLE_IMAGE + m_facing);
}

StreetStage::Flow StreetStage::stopForLoading(Step next) {
  m_machine.destroyAll();
  m_playerX = xBob(PLAYER);
  global(amal::RB) = word((global(amal::RB) / 4) * 4);
  m_bobs.offAll();
  m_step = next;
  autoback([](core::IndexedSurface &) {});
  return Flow::Yield;
}

void StreetStage::streetSetup() {
  m_bobs.set(PLAYER_BLOOD, 1000, 100, HIDDEN_IMAGE);
  m_bobs.set(ENEMY_BLOOD, 1000, 100, HIDDEN_IMAGE);
  m_bobs.set(INDICATOR, 242 + 164 * actors::amosBool(stage() == 2), 32,
             HIDDEN_IMAGE);
  for (int channel = 1; channel <= 3; ++channel) {
    m_machine.bind(channel, &m_bobs.object(PLAYER));
  }
  m_machine.bind(PLAYER_BLOOD_CHANNEL, &m_bobs.object(PLAYER_BLOOD));
  m_machine.bind(ENEMY_BLOOD_CHANNEL, &m_bobs.object(ENEMY_BLOOD));
  m_machine.bind(INDICATOR_CHANNEL, &m_bobs.object(INDICATOR));

  m_machine.bind(SCREEN_SHAKE_CHANNEL, &m_screenDisplay);
  m_machine.create(SCREEN_SHAKE_CHANNEL,
                   actors::screenShake(m_session.version));
  const auto player = actors::streetPlayer(stage(), m_session.version);
  m_machine.create(1, player.locomotion);
  m_machine.create(2, player.damage);
  m_machine.create(3, player.clamp);
  m_machine.create(PLAYER_BLOOD_CHANNEL, actors::playerBlood());
  m_machine.create(ENEMY_BLOOD_CHANNEL, actors::enemyBlood(m_session.version));
  m_machine.startAll();

  m_panel->score(stats());
  for (int i = 0; i <= 2; ++i) {
    reg(5 + i * 2, 7) = word(m_energy[static_cast<std::size_t>(i + 1)]);
  }
  m_host.playSample(2, OPENING_SHOUT, systems::audio::Mixer::ALL_VOICES);
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
    global(amal::RD) = 0;
    m_machine.start(1);
  }
  if (reg(2, 1) == 9) {
    reg(2, 1) = 0;
    global(amal::RD) = 0;
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
      int x = reg(p, 6) - 57 - 18 * actors::amosBool(reg(p, 1) < 0);
      if (stage() == 2) {
        x = x < -40 ? 400 : std::max(16, x);
      } else {
        x = x > 280 ? 400 : std::min(200, x);
      }
      if (pasteStalled(x, reg(p, 7) - 17, image)) {
        m_step = Step::RefereeCorpseStamped;
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
    return (xBob(i) < xBob(PLAYER) && global(amal::RC) != 0) ||
           (xBob(i) > xBob(PLAYER) && global(amal::RC) == 0);
  };

  if (global(amal::RD) == 2) {
    bobCol(PLAYER);
    for (int i = 2; i <= 4; ++i) {
      if (col(i) && reg(i * 2 + 1, 4) == 1 && yBob(PLAYER) < yBob(i) + 6 &&
          yBob(PLAYER) > yBob(i) - 6 && xBob(PLAYER) < 240 &&
          xBob(PLAYER) > 42 && global(amal::RC) != reg(i * 2, 2) &&
          xBob(PLAYER) <
              xBob(i) - 25 * actors::amosBool(global(amal::RC) == 0) &&
          xBob(PLAYER) >
              xBob(i) + 25 * actors::amosBool(global(amal::RC) != 0)) {
        const int p = i * 2 + 1;
        m_bobs.setPosition(
            i, xBob(PLAYER) + 24 + 48 * actors::amosBool(global(amal::RC) == 0),
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

  if (global(amal::RD) == 1 || global(amal::RD) == 2) {
    bobCol(PLAYER);
    for (int i = 2; i <= 4; ++i) {
      if (col(i) && inFront(i) && yBob(i) == yBob(PLAYER)) {
        const int p = i * 2 + 1;
        m_machine.freeze(i * 2);
        reg(p, 1) = word(0x8000 - global(amal::RC));
        reg(p, 2) = 1;
        reg(p, 0) = global(amal::RD);
        break;
      }
    }
  }

  if (global(amal::RD) == 3) {
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

  if (global(amal::RD) == 4) {
    bobCol(PLAYER);
    for (int i = 2; i <= 4; ++i) {
      if (col(i) && global(amal::RB) == yBob(i) && reg(i * 2 + 1, 2) != 1 &&
          reg(i * 2, 3) == 0 && inFront(i)) {
        const int p = i * 2 + 1;
        m_machine.freeze(i * 2);
        reg(p, 1) = word(0x8000 - global(amal::RC));
        reg(p, 3) = word(16 + 32 * actors::amosBool(reg(p, 1) == 0));
        reg(p, 2) = 1;
        reg(p, 0) = 4;
      }
    }
  }

  if (global(amal::RD) == 5) {
    bobCol(PLAYER);
    for (int i = 2; i <= 4; ++i) {
      if (col(i) && global(amal::RB) == yBob(i) && reg(i * 2 + 1, 2) != 1) {
        const int p = i * 2 + 1;
        m_machine.freeze(i * 2);
        reg(p, 1) = reg(i * 2, 2);
        reg(p, 3) = word(16 + 32 * actors::amosBool(reg(p, 1) == 0));
        reg(p, 2) = 1;
        reg(p, 0) = 5;
      }
    }
  }

  if (global(amal::RD) == 6) {
    bobCol(PLAYER);
    for (int i = 2; i <= 4; ++i) {
      if (reg(i * 2 + 1, 5) == 0 && global(amal::RC) != reg(i * 2, 2) &&
          col(i) && reg(2, 5) == 0 && yBob(PLAYER) == yBob(i) && inFront(i)) {
        const int p = i * 2 + 1;
        m_machine.freeze(i * 2);
        m_machine.freeze(1);
        reg(p, 1) = word(0x8000 - global(amal::RC));
        m_bobs.setX(i, xBob(PLAYER) + 40 +
                           80 * actors::amosBool(global(amal::RC) != 0));
        reg(p, 6) = word(16 + 32 * actors::amosBool(global(amal::RC) == 0));
        reg(p, 2) = 1;
        reg(p, 3) = word(16 + 32 * actors::amosBool(reg(p, 1) == 0));
        reg(2, 5) = 1;
        reg(p, 0) = 6;
        break;
      }
    }
  }

  for (int i = 2; i <= 4; ++i) {
    if (bobCol(i) && col(PLAYER) && global(amal::RD) == 0 &&
        yBob(PLAYER) == yBob(i) && reg(2, 1) == 0 && reg(i * 2, 9) > 0 &&
        reg(i * 2, 9) < 4 && reg(i * 2 + 1, 0) == 0) {
      if ((reg(i * 2, 2) == 0 && xBob(PLAYER) > xBob(i)) ||
          (reg(i * 2, 2) != 0 && xBob(PLAYER) < xBob(i))) {
        m_machine.freeze(1);
        global(amal::RV) = 0;
        reg(2, 5) = 0;
        reg(2, 9) = 50;
        reg(5, 9) = 50;
        reg(7, 9) = 50;
        reg(9, 9) = 50;
        m_bobs.setX(i, xBob(i) + 8 + 16 * actors::amosBool(reg(2 * i, 2) == 0));
        global(amal::RC) = word(0x8000 - reg(2 * i, 2));
        reg(2, 2) = word(32 + 64 * actors::amosBool(global(amal::RC) == 0));
        reg(2, 1) = reg(i * 2, 9);
      }
    }
  }

  if (global(amal::RW) != 0) {
    const int request = global(amal::RW);
    global(amal::RW) = 0;
    playRouted(request, PRIORITY_VOICE);
  } else if (global(amal::RE) != 0) {
    const int request = global(amal::RE);
    global(amal::RE) = 0;
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
      reg(channel, 3) = word(16 + 32 * actors::amosBool(reg(channel, 1) == 0));
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

  if (m_escape || global(amal::RG) == -2) {
    gameOver();
    return Flow::Continue;
  }
  if (global(amal::RI) < 0) {
    m_step = Step::AdvanceWait;
    return Flow::Continue;
  }
  if (global(amal::RI) == 0) {
    global(amal::RI) = -1;
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
    if (pasteStalled(x, y, image)) {
      m_step = Step::RefereeBloodStamped;
      return Flow::Yield;
    }
    hidePastedBlood(i);
  }
  return refereeTail();
}

StreetStage::Flow StreetStage::refereeBloodStamped() {
  hidePastedBlood(m_index);
  ++m_index;
  m_step = Step::Referee;
  return refereeBlood();
}

void StreetStage::hidePastedBlood(int bob) {
  if (m_session.version == GameVersion::V10) {
    m_bobs.setImage(bob, HIDDEN_IMAGE);
  }
}

StreetStage::Flow StreetStage::refereeTail() {
  updatePanel();
  sys();
  m_step = Step::Referee;
  return endOfPass();
}

StreetStage::Flow StreetStage::advanceWait() {
  if (global(amal::RZ) != 0 || global(amal::RY) != 0 || global(amal::RM) != 0) {
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
  m_machine.create(INDICATOR_CHANNEL, actors::indicatorArrow(m_facing));
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
  if (global(amal::RE) != 0) {
    m_host.playSample(2, global(amal::RE), PRIORITY_VOICE);
    global(amal::RE) = 0;
  }
  if (m_columnInChunk == COLUMNS_PER_CHUNK) {
    m_machine.freezeAll();
    m_bobs.setPosition(PLAYER, xBob(PLAYER), (global(amal::RB) / 4) * 4);
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
              iBob(PLAYER) < 17 && global(amal::RD) == 0;
    bias = 6 - 2 * actors::amosBool(xBob(PLAYER) < 152);
  } else {
    walking = xBob(PLAYER) > 152 && joystick < 16 && (joystick & 8) &&
              iBob(PLAYER) < 17 && global(amal::RD) == 0;
    bias = 6 - 2 * actors::amosBool(xBob(PLAYER) > 164);
  }
  if (!walking) {
    return advanceTail();
  }
  reg(1, 1) = word(bias);
  m_scrollPhase = m_scrollPhase + 1 > 1 ? 0 : m_scrollPhase + 1;
  if (m_scrollPhase == 0) {
    autoback([column = m_columns.at(static_cast<std::size_t>(m_columnInChunk)),
              x = stage() == 2 ? 0 : 304](core::IndexedSurface &surface) {
      surface.unpack(column, x, 0);
    });
    ++m_columnInChunk;
    ++m_columnsWalked;
    m_step = Step::AdvanceScroll;
    return Flow::Yield;
  }
  return advanceScroll();
}

StreetStage::Flow StreetStage::advanceScroll() {
  m_step = Step::AdvanceWalked;
  scrollStep();
  return Flow::Yield;
}

StreetStage::Flow StreetStage::advanceWalked() {
  reg(1, 1) = 0;
  return advanceTail();
}

StreetStage::Flow StreetStage::advanceTail() {
  if (m_columnsWalked > 10 && m_columnsWalked == m_script.length - 1) {
    m_step = Step::AdvanceLeave;
    scrollStep();
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
  return grabPlayer() ? Flow::Yield : Flow::Continue;
}

StreetStage::Flow StreetStage::advanceLeavePasted() {
  m_session.streetExit.emplace(session::StreetExit{
      m_screen, *m_block, m_playerX, m_energyShown, m_killsShown, m_buffer});
  m_block.reset();
  m_outcome = Outcome::Cleared;
  m_step = Step::Finished;
  return Flow::Yield;
}

StreetStage::Flow StreetStage::spawnFlushed() {
  m_step = Step::SpawnPasted;
  return grabPlayer() ? Flow::Yield : Flow::Continue;
}

StreetStage::Flow StreetStage::spawnPasted() {
  test();
  m_panel->showWaiting();
  const core::Wave &wave = m_script.waves[static_cast<std::size_t>(m_nextWave)];
  global(amal::RI) = 3;

  for (int i = 1; i <= 3; ++i) {
    const int resident = m_resident[static_cast<std::size_t>(i)];
    m_needed[static_cast<std::size_t>(i)] =
        actors::amosBool(wave.slots[0].spriteSet == resident ||
                         wave.slots[1].spriteSet == resident ||
                         wave.slots[2].spriteSet == resident);
  }
  int slot = 0;
  for (const core::EnemySlot &enemy : wave.slots) {
    bool missing = false;
    for (int i = 1; i <= 3; ++i) {
      if (enemy.spriteSet != m_resident[static_cast<std::size_t>(i)] &&
          enemy.spriteSet != core::EnemySlot::EMPTY) {
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
  const core::Wave &wave = m_script.waves[static_cast<std::size_t>(m_nextWave)];
  for (int j = 2; j <= 4; ++j) {
    const core::EnemySlot &enemy = wave.slots[static_cast<std::size_t>(j - 2)];
    m_machine.bind(j * 2, &m_bobs.object(j));
    m_machine.bind(j * 2 + 1, &m_bobs.object(j));
    if (enemy.spriteSet == core::EnemySlot::EMPTY) {
      m_bobs.set(j, 1000, 300, HIDDEN_IMAGE);
      m_machine.create(j * 2, actors::idle(m_session.version));
      m_machine.create(j * 2 + 1, actors::idle(m_session.version));
      global(amal::RI) = word(global(amal::RI) - 1);
      continue;
    }
    m_energy[static_cast<std::size_t>(j - 1)] = enemy.energy;
    m_aggression[static_cast<std::size_t>(j - 1)] = enemy.aggression;
    m_bobs.set(j, enemy.x, enemy.y, HIDDEN_IMAGE);
    const int base = 0 -
                     25 * actors::amosBool(m_resident[2] == enemy.spriteSet) -
                     50 * actors::amosBool(m_resident[3] == enemy.spriteSet);
    const auto programs = actors::enemy(base, enemy.type, m_session.version);
    m_machine.create(j * 2, programs.walk);
    m_machine.create(j * 2 + 1, programs.damage);
  }
  m_host.yield();

  ++m_nextWave;
  ++m_wavesSpawned;
  m_bobs.set(PLAYER, m_playerX, (global(amal::RB) / 4) * 4,
             IDLE_IMAGE + m_facing);
  putBlock(*m_block);
  m_block.reset();
}

void StreetStage::gameOver() {
  Stage::gameOver();
  m_step = Step::GameOver;
}

void StreetStage::runBasic(const StreetInput &input) {
  Flow flow = Flow::Continue;
  while (flow == Flow::Continue && m_frame >= m_resumeFrame) {
    switch (m_step) {
    case Step::Start:
      if (m_session.fromBonusDrive) {
        m_session.fromBonusDrive = false;
        openScreens(true);
        flow = stageInit();
        break;
      }
      newGame();
      gameInit();
      m_resumeFrame =
          m_frame + SCREEN_OPEN_VBLS + DOUBLE_BUFFER_VBLS + SCREEN_OPEN_VBLS;
      m_step = Step::GameInitialized;
      flow = Flow::Yield;
      break;
    case Step::GameInitialized:
      m_panelShown = true;
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
    case Step::GameOver:
      flow = advanceGameOver();
      break;
    case Step::Finished:
      flow = Flow::Yield;
      break;
    }
  }
}

} // namespace openfranko::src::engine::street::scenes
