#include "BossStage.h"

#include "../effects/AmigaDisplay.h"

#include "../amal/Actors.h"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace openfranko::src::engine::street {
namespace {

using amal::actors::amosBool;

constexpr int RB = 1;
constexpr int RC = 2;
constexpr int RD = 3;
constexpr int RE = 4;
constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RI = 8;
constexpr int RN = 13;
constexpr int RO = 14;
constexpr int RP = 15;
constexpr int RQ = 16;
constexpr int RR = 17;
constexpr int RS = 18;
constexpr int RT = 19;
constexpr int RU = 20;
constexpr int RV = 21;
constexpr int RW = 22;
constexpr int RX = 23;

constexpr int PLAYER = 1;
constexpr int BOSS = 2;
constexpr int SPECTATOR = 3;
constexpr int PLAYER_BUBBLE = 4;
constexpr int BOSS_BUBBLE = 5;
constexpr int BOSS_BLOOD = 10;
constexpr int PLAYER_BLOOD = 11;
constexpr int INDICATOR = 12;
constexpr int HIDDEN_IMAGE = 10;
constexpr int SPLAT_IMAGE = 9;
constexpr int IDLE_IMAGE = 17;
constexpr int BOSS_BUBBLE_IMAGE = 91;

constexpr int SCREEN_SHAKE_CHANNEL = 0;
constexpr int PLAYER_WALK_CHANNEL = 1;
constexpr int PLAYER_DAMAGE_CHANNEL = 2;
constexpr int PLAYER_CLAMP_CHANNEL = 3;
constexpr int BOSS_WALK_CHANNEL = 4;
constexpr int BOSS_DAMAGE_CHANNEL = 5;
constexpr int SPECTATOR_CHANNEL = 6;
constexpr int PLAYER_TALK_CHANNEL = 7;
constexpr int BOSS_TALK_CHANNEL = 8;
constexpr int INDICATOR_CHANNEL = 13;
constexpr int ENEMY_BLOOD_CHANNEL = 14;
constexpr int PLAYER_BLOOD_CHANNEL = 15;

constexpr int AUTOBACK_VBLS = 3;
constexpr int GAME_OVER_WAIT = 200;
constexpr int FULL_ENERGY = 64;
constexpr int EXTRA_LIFE_STEP = 40;
constexpr int MUSIC_VOLUME = 30;
constexpr int PRIORITY_VOICE = 1;
constexpr int PLAYER_SAMPLE_BANK = 2;
constexpr int BOSS_SAMPLE_BANK = 4;
constexpr int FIGHT_SHOUT = 9;
constexpr int WALK_OFF_SAMPLE = 4;
constexpr int FINISHING_SAMPLE = 9;

constexpr int STREET_Y = 172;
constexpr int APPROACH_TOP = 164;
constexpr int ARENA_TOP = 124;
constexpr int DIALOGUE_START = 1;
constexpr int DIALOGUE_OVER = 99;
constexpr int SPECIAL_RANGE = 40;
constexpr int WALK_OFF_DISTANCE = 340;
constexpr int FINISHING_STAMPS = 100;
constexpr int POSE_WAIT = 50;
constexpr int LIFT_WAIT = 150;
constexpr int THROW_WAIT = 60;
constexpr int SHOUT_WAIT = 50;
constexpr int BOSS_THROWN_SAMPLE = 9;
constexpr int VICTORY_SAMPLE = 8;

constexpr int TAUNT_POSE = 78;
constexpr int TAUNT_SAMPLE = 7;
constexpr int TAUNT_EVERY = 15;
constexpr int CHILD_STANDS = 71;
constexpr int CHILD_BODY = 80;
constexpr int CHILD_BODY_X = 232;
constexpr int CHILD_BODY_Y = 180;
constexpr int CHILD_PUNCH = 81;
constexpr int CHILD_FLEX = 74;
constexpr int CHILD_BEATS = 3;
constexpr int CHILD_WAIT = 15;
constexpr int CHILD_CRY_SAMPLE = 7;

constexpr int RAILING_X = 208;
constexpr int RAILING_Y = 143;
constexpr int RAILING_TILE_X = 183;
constexpr int RAILING_TILE_Y = 60;
constexpr int RAILING_SIT = 82;
constexpr int RAILING_BROKEN = 85;
constexpr int REST_BUBBLE = 93;
constexpr int CURSE_BUBBLE = 94;
constexpr int CURSE_BUBBLE_X = 220;
constexpr int CURSE_BUBBLE_Y = 64;
constexpr int REST_WAIT = 50;
constexpr int SIT_WAIT = 100;
constexpr int FALL_WAIT = 15;
constexpr int FALLEN_WAIT = 25;
constexpr int VICTORY_POSE_DELAY = 90;
constexpr int POSE_IMAGE = 38;
constexpr int GRIN_IMAGE = 39;
constexpr int POSE_HOLD = 30;
constexpr int GRIN_HOLD = 40;
constexpr int CURSE_SAMPLE = 9;
constexpr int GRIN_SAMPLE = 3;

int16_t word(int value) { return static_cast<int16_t>(value); }

int clampBound(int stage) { return stage == 2 ? 48 : 272; }

} // namespace

BossStage::BossStage(StreetHost &host, GameSession &session,
                     effects::GameOptions &options)
    : m_host(host), m_session(session), m_options(options),
      m_machine(session.registers), m_screen(SCREEN_WIDTH, SCREEN_HEIGHT),
      m_buffer(m_screen),
      m_screenDisplay{DISPLAY_X,
                      static_cast<int16_t>(playDisplayY(stageLayout(options))),
                      0},
      m_palette(levelPalette(options.mono)), m_panelPalette(panelPalette()) {
  m_session.border = STAGE_BORDER;
}

void BossStage::advance(const StreetInput &input) {
  if (m_step == Step::Finished) {
    return;
  }
  ++m_frame;
  if (input.key != SystemKey::None) {
    m_pendingKey = input.key;
  }
  m_buffer.vbl();
  m_machine.setJoystick(input.joystick);
  m_machine.tick();
  if (m_buffer.isAutobacking()) {
    m_buffer.autobackStep(m_bobs, m_images);
  } else {
    m_buffer.test(m_bobs, m_images);
  }
  runBasic(input);
  if (!m_buffer.isAutobacking()) {
    m_buffer.test(m_bobs, m_images);
  }
}

void BossStage::compose(std::vector<uint32_t> &frame) const {
  composeFrame(frame, m_screenShown ? &m_buffer.shown() : nullptr, m_palette,
               m_screenDisplay, m_screenOffsetX,
               m_panelShown ? m_panel.get() : nullptr, m_panelPalette,
               stageLayout(m_options));
}

BossStage::Outcome BossStage::outcome() const { return m_outcome; }

const BobLayer &BossStage::bobs() const { return m_bobs; }

const IndexedSurface &BossStage::screen() const { return m_screen; }

const IndexedSurface &BossStage::display() const { return m_buffer.shown(); }

const StatusPanel *BossStage::panel() const { return m_panel.get(); }

bool BossStage::isScreenShown() const { return m_screenShown; }

bool BossStage::isPanelShown() const { return m_panelShown; }

amal::Machine &BossStage::machine() { return m_machine; }

int BossStage::columnsWalked() const { return m_columnsWalked; }

bool BossStage::isApproaching() const {
  return m_step == Step::Approach || m_step == Step::ApproachUnpacked ||
         m_step == Step::ApproachScrolled;
}

bool BossStage::isTalking() const {
  return m_step == Step::ChildPasted || m_step == Step::ChildHit ||
         m_step == Step::ChildRaised || m_step == Step::ChildCried ||
         m_step == Step::Dialogue;
}

bool BossStage::isFighting() const {
  return m_step == Step::Fight || m_step == Step::FightBloodStamped;
}

bool BossStage::isFinishing() const {
  return m_step == Step::FinishWalkedToBoss || m_step == Step::FinishPosed ||
         m_step == Step::FinishStamped || m_step == Step::FinishPosedBack ||
         m_step == Step::FinishWalkedOff || m_step == Step::LiftWalkedToBoss ||
         m_step == Step::LiftRaised || m_step == Step::LiftThrown ||
         m_step == Step::LiftDone || isAtRailing() || m_step == Step::Cleared;
}

bool BossStage::isAtRailing() const {
  return m_step == Step::RailingSpeech || m_step == Step::RailingWaitFire ||
         m_step == Step::RailingReached || m_step == Step::RailingSat ||
         m_step == Step::RailingSitting || m_step == Step::RailingCurse ||
         m_step == Step::RailingFall || m_step == Step::RailingFell ||
         m_step == Step::RailingFallNext || m_step == Step::RailingQuiet ||
         m_step == Step::RailingPose || m_step == Step::RailingGrin ||
         m_step == Step::RailingGrinned || m_step == Step::RailingDone;
}

int16_t &BossStage::global(int index) {
  return m_machine.globalRegister(index);
}

int16_t &BossStage::reg(int channel, int index) {
  return m_machine.channelRegister(channel, index);
}

int BossStage::xBob(int number) const { return m_bobs.x(number); }

int BossStage::yBob(int number) const { return m_bobs.y(number); }

int BossStage::iBob(int number) const { return m_bobs.image(number); }

bool BossStage::bobCol(int number) { return m_bobs.collide(number, m_images); }

bool BossStage::col(int number) const { return m_bobs.collided(number); }

int BossStage::stage() const { return m_session.registers[RO]; }

StatusPanel::Stats BossStage::stats() const {
  const amal::Registers &registers = m_session.registers;
  return {registers[RF], registers[RO], registers[RN], registers[RG]};
}

void BossStage::stall() { m_resumeFrame = m_frame + AUTOBACK_VBLS; }

void BossStage::autoback(DoubleBuffer::Op op) {
  op(m_screen);
  m_buffer.autoback(std::move(op));
  stall();
}

bool BossStage::pasteStalled(int x, int y, int image) {
  if (!BobLayer::paste(m_screen, m_images, x, y, image)) {
    return false;
  }
  m_buffer.autoback([this, x, y, image](IndexedSurface &surface) {
    BobLayer::paste(surface, m_images, x, y, image);
  });
  stall();
  return true;
}

BossStage::Flow BossStage::waitFrames(int frames, Step next) {
  m_step = next;
  if (frames <= 0) {
    return Flow::Continue;
  }
  m_resumeFrame = m_frame + frames;
  return Flow::Yield;
}

BossStage::Flow BossStage::endOfPass() const {
  return m_passFrame == m_frame ? Flow::Yield : Flow::Continue;
}

void BossStage::playRequest(int request) {
  const int boss = amosBool(request > 8);
  m_host.playSample(PLAYER_SAMPLE_BANK - 2 * boss, request + 8 * boss,
                    PRIORITY_VOICE);
}

BossStage::Flow BossStage::init() {
  if (!m_session.streetExit) {
    throw std::logic_error("BossStage needs the screen the street left");
  }
  m_screen = m_session.streetExit->screen;
  m_buffer = m_session.streetExit->buffer ? *m_session.streetExit->buffer
                                          : DoubleBuffer(m_screen);
  m_block.emplace(m_session.streetExit->block);
  m_playerX = m_session.streetExit->playerX;
  m_energyShown = m_session.streetExit->energyShown;
  m_killsShown = m_session.streetExit->killsShown;
  m_session.streetExit.reset();
  m_facing = -32768 * amosBool(stage() == 2);
  m_screenOffsetX = stage() == 2 ? 16 : 0;
  m_panel = std::make_unique<StatusPanel>(
      m_host.loadPanelPicture(StreetStage::LOADING_STRIP),
      m_host.loadPanelPicture(StreetStage::PANEL_ARTWORK));

  m_host.stopMusic();
  m_images.clear();
  m_loading.queue([this] { m_host.loadMusic(stage() + 603); });
  return load(Step::BossMusic);
}

BossStage::Flow BossStage::bossMusic() {
  m_host.playMusic();
  m_host.setMusicVolume(m_options.music ? MUSIC_VOLUME : 0);
  m_loading.queue(
      [this] { m_columns = m_host.loadScenery(stage() * 10 + 310); });
  m_loading.queue([this] { m_images.load(1, m_host.loadSpriteSet(0, 0)); });
  const int player = 254 - 5 * global(RQ);
  m_loading.queue([this, player] {
    m_images.load(11, m_host.loadSpriteSet(player, PLAYER_SAMPLE_BANK));
  });
  m_loading.queue([this, player] {
    m_images.load(38, m_host.loadSpriteSet(player - stage(), 0));
  });
  m_loading.queue([this] {
    m_images.load(43, m_host.loadSpriteSet(201 - stage(), BOSS_SAMPLE_BANK));
  });
  return load(Step::BossLoaded);
}

void BossStage::bossLoaded() {
  m_columnsWalked = 0;
  m_panel->score(stats());
  m_bobs.set(PLAYER, m_playerX, (global(RB) / 4) * 4, IDLE_IMAGE + m_facing);
  m_block->put(m_screen);
  m_block->put(m_buffer.logic());
  m_buffer.swap();
  m_block->put(m_buffer.logic());
  m_buffer.swap();
  m_block.reset();
}

BossStage::Flow BossStage::load(Step next) {
  m_afterLoading = next;
  m_step = Step::Loading;
  return Flow::Continue;
}

void BossStage::setUp() {
  m_panel->showWaiting();
  m_scrollPhase = 1;
  for (int channel = PLAYER_WALK_CHANNEL; channel <= PLAYER_CLAMP_CHANNEL;
       ++channel) {
    m_machine.bind(channel, &m_bobs.object(PLAYER));
  }
  m_machine.bind(BOSS_WALK_CHANNEL, &m_bobs.object(BOSS));
  m_machine.bind(BOSS_DAMAGE_CHANNEL, &m_bobs.object(BOSS));
  m_machine.bind(SPECTATOR_CHANNEL, &m_bobs.object(SPECTATOR));
  m_machine.bind(PLAYER_TALK_CHANNEL, &m_bobs.object(PLAYER_BUBBLE));
  m_machine.bind(BOSS_TALK_CHANNEL, &m_bobs.object(BOSS_BUBBLE));
  m_machine.bind(PLAYER_BLOOD_CHANNEL, &m_bobs.object(PLAYER_BLOOD));
  m_machine.bind(ENEMY_BLOOD_CHANNEL, &m_bobs.object(BOSS_BLOOD));
  m_machine.bind(INDICATOR_CHANNEL, &m_bobs.object(INDICATOR));

  m_bobs.set(PLAYER, m_playerX, (global(RB) / 4) * 4, IDLE_IMAGE + m_facing);
  m_bobs.set(INDICATOR, 242 + 164 * amosBool(stage() == 2), 32, HIDDEN_IMAGE);
  m_bobs.set(PLAYER_BLOOD, 1000, yBob(PLAYER_BLOOD), HIDDEN_IMAGE);
  m_bobs.set(BOSS_BLOOD, 1000, yBob(BOSS_BLOOD), HIDDEN_IMAGE);
  for (int bob = BOSS; bob <= BOSS_BUBBLE; ++bob) {
    m_bobs.set(bob, 1000, STREET_Y, HIDDEN_IMAGE);
  }
  global(RT) = 0;
  global(RU) = 0;
  global(RX) = 0;

  m_machine.bind(SCREEN_SHAKE_CHANNEL, &m_screenDisplay);
  m_machine.create(SCREEN_SHAKE_CHANNEL, amal::actors::screenShake());
  m_machine.create(PLAYER_BLOOD_CHANNEL, amal::actors::playerBlood());
  m_machine.create(ENEMY_BLOOD_CHANNEL, amal::actors::enemyBlood());
  const auto player = amal::actors::bossPlayer(stage());
  m_machine.create(PLAYER_WALK_CHANNEL, player.locomotion);
  m_machine.create(PLAYER_DAMAGE_CHANNEL, player.damage);
  m_machine.create(PLAYER_CLAMP_CHANNEL, player.clamp);
  const auto boss = amal::actors::boss(stage());
  m_machine.create(BOSS_WALK_CHANNEL, boss.walk);
  m_machine.create(BOSS_DAMAGE_CHANNEL, boss.damage);
  const std::string spectator = amal::actors::spectator(stage());
  if (!spectator.empty()) {
    m_machine.create(SPECTATOR_CHANNEL, spectator);
  }
  const auto dialogue = amal::actors::dialogue(stage());
  m_machine.create(PLAYER_TALK_CHANNEL, dialogue.player);
  m_machine.create(BOSS_TALK_CHANNEL, dialogue.boss);
  m_machine.startAll();

  reg(PLAYER_WALK_CHANNEL, 2) = APPROACH_TOP;
  reg(PLAYER_CLAMP_CHANNEL, 0) = word(clampBound(stage()));
  global(RI) = 1;
  m_machine.create(INDICATOR_CHANNEL, amal::actors::indicatorArrow(m_facing));
  m_machine.start(INDICATOR_CHANNEL);
  m_panel->score(stats());
  m_bobs.set(BOSS_BLOOD, 120, 24, HIDDEN_IMAGE);
  m_bobs.set(PLAYER_BLOOD, 120, 24, HIDDEN_IMAGE);
}

BossStage::Flow BossStage::approachTop(const StreetInput &input) {
  m_passFrame = m_frame;
  const int joystick = input.joystick;
  bool walking = false;
  int bias = 0;
  if (stage() == 2) {
    walking = xBob(PLAYER) < 232 && joystick < 16 && (joystick & 4) &&
              global(RD) == 0 && iBob(PLAYER) < 17;
    bias = 6 - 2 * amosBool(xBob(PLAYER) < 220);
  } else {
    walking = xBob(PLAYER) > 96 && joystick < 16 && (joystick & 8) &&
              global(RD) == 0 && iBob(PLAYER) < 17;
    bias = 6 - 2 * amosBool(xBob(PLAYER) > 108);
  }
  if (!walking) {
    return approachTail();
  }
  reg(PLAYER_WALK_CHANNEL, 1) = word(bias);
  m_scrollPhase = m_scrollPhase + 1 > 1 ? 0 : m_scrollPhase + 1;
  if (m_scrollPhase == 0) {
    autoback([column = m_columns.at(static_cast<std::size_t>(m_columnsWalked)),
              x = stage() == 2 ? 0 : 304](IndexedSurface &surface) {
      surface.unpack(column, x, 0);
    });
    ++m_columnsWalked;
    m_step = Step::ApproachUnpacked;
    return Flow::Yield;
  }
  return approachScroll();
}

BossStage::Flow BossStage::approachScroll() {
  global(RX) = 1;
  m_step = Step::ApproachScrolled;
  scrollStep();
  return Flow::Yield;
}

BossStage::Flow BossStage::approachScrolled() {
  reg(PLAYER_CLAMP_CHANNEL, 0) =
      word(reg(PLAYER_CLAMP_CHANNEL, 0) + (stage() == 2 ? 4 : -4));
  reg(PLAYER_WALK_CHANNEL, 1) = 0;
  return approachTail();
}

BossStage::Flow BossStage::approachTail() {
  if (m_columnsWalked == APPROACH_COLUMNS) {
    global(RX) = 2;
    startDialogue();
    if (stage() == 3) {
      return beatChild();
    }
    beginTalk();
    m_step = Step::Dialogue;
    return Flow::Continue;
  }
  if (stage() == 3 && iBob(BOSS) == TAUNT_POSE && global(RE) == 0 &&
      (!m_lastTaunt || m_frame - *m_lastTaunt > TAUNT_EVERY)) {
    m_lastTaunt = m_frame;
    m_host.playSample(PLAYER_SAMPLE_BANK, TAUNT_SAMPLE, PRIORITY_VOICE);
  }
  if (global(RE) != 0) {
    m_host.playSample(PLAYER_SAMPLE_BANK, global(RE), PRIORITY_VOICE);
    global(RE) = 0;
  }
  sys();
  if (m_escape) {
    gameOver();
    return Flow::Continue;
  }
  m_step = Step::Approach;
  return endOfPass();
}

void BossStage::startDialogue() {
  m_machine.destroy(INDICATOR_CHANNEL);
  m_bobs.setImage(INDICATOR, HIDDEN_IMAGE);
  m_machine.freeze(PLAYER_WALK_CHANNEL);
  m_machine.freeze(BOSS_WALK_CHANNEL);
  m_bobs.setImage(PLAYER, IDLE_IMAGE + m_facing);
}

void BossStage::beginTalk() {
  global(RT) = DIALOGUE_START;
  if (stage() == 1) {
    m_bobs.set(BOSS_BUBBLE, 176, 40, BOSS_BUBBLE_IMAGE);
  }
}

BossStage::Flow BossStage::beatChild() {
  m_bobs.setImage(BOSS, CHILD_STANDS);
  m_step = Step::ChildPasted;
  return pasteStalled(CHILD_BODY_X, CHILD_BODY_Y, CHILD_BODY) ? Flow::Yield
                                                              : Flow::Continue;
}

BossStage::Flow BossStage::childRaised() {
  if (++m_index <= CHILD_BEATS) {
    m_bobs.setImage(BOSS, CHILD_STANDS);
    return waitFrames(CHILD_WAIT, Step::ChildHit);
  }
  m_bobs.setImage(BOSS, CHILD_FLEX);
  m_host.playSample(BOSS_SAMPLE_BANK, CHILD_CRY_SAMPLE, PRIORITY_VOICE);
  return waitFrames(CHILD_WAIT, Step::ChildCried);
}

BossStage::Flow BossStage::dialogue() {
  if (global(RT) != DIALOGUE_OVER) {
    return Flow::Yield;
  }
  reg(PLAYER_CLAMP_CHANNEL, 0) = word(clampBound(stage()));
  m_machine.startAll();
  m_host.playSample(PLAYER_SAMPLE_BANK, FIGHT_SHOUT, PRIORITY_VOICE);
  reg(BOSS_DAMAGE_CHANNEL, 7) = BOSS_ENERGY;
  reg(PLAYER_WALK_CHANNEL, 2) = ARENA_TOP;
  m_step = Step::Fight;
  return Flow::Continue;
}

BossStage::Flow BossStage::fightTop() {
  m_passFrame = m_frame;
  const auto idle = [this]() {
    return reg(BOSS_WALK_CHANNEL, 8) == 0 && reg(BOSS_WALK_CHANNEL, 1) == 0;
  };
  const auto clear = [this]() {
    return yBob(PLAYER) == yBob(BOSS) && reg(PLAYER_DAMAGE_CHANNEL, 1) == 0 &&
           reg(BOSS_DAMAGE_CHANNEL, 0) == 0;
  };
  const auto facingPlayer = [this]() {
    return (reg(BOSS_WALK_CHANNEL, 2) == 0 && xBob(PLAYER) > xBob(BOSS)) ||
           (reg(BOSS_WALK_CHANNEL, 2) != 0 && xBob(PLAYER) < xBob(BOSS));
  };
  const auto facingBoss = [this]() {
    return (global(RC) == 0 && xBob(PLAYER) < xBob(BOSS)) ||
           (global(RC) != 0 && xBob(PLAYER) > xBob(BOSS));
  };
  const auto inFront = [this]() {
    return (xBob(BOSS) < xBob(PLAYER) && global(RC) != 0) ||
           (xBob(BOSS) > xBob(PLAYER) && global(RC) == 0);
  };
  const auto snapBoss = [this]() {
    m_machine.freeze(PLAYER_WALK_CHANNEL);
    global(RV) = 0;
    reg(PLAYER_DAMAGE_CHANNEL, 5) = 0;
    m_bobs.setX(BOSS, xBob(PLAYER) + 32 +
                          64 * amosBool(reg(BOSS_WALK_CHANNEL, 2) == 0));
    global(RC) = word(0x8000 - reg(BOSS_WALK_CHANNEL, 2));
  };

  const bool taunt = m_host.random(40) == 0;
  if (taunt && reg(BOSS_WALK_CHANNEL, 1) == 0 &&
      reg(BOSS_WALK_CHANNEL, 8) == 0) {
    reg(BOSS_WALK_CHANNEL, 1) = word(m_host.random(2) + 2);
    reg(BOSS_WALK_CHANNEL, 6) =
        word(32 + 64 * amosBool(reg(BOSS_WALK_CHANNEL, 2) != 0));
  }
  const bool jumping = global(RD) == 4 || global(RD) == 5;
  const bool duck = m_host.random(10) > 3;
  if (jumping && duck) {
    reg(BOSS_WALK_CHANNEL, 1) = 1;
  }

  const bool special = m_host.random(10) == 0;
  if (special && idle() && clear()) {
    const bool close =
        (reg(BOSS_WALK_CHANNEL, 2) == 0 && xBob(PLAYER) > xBob(BOSS) &&
         xBob(PLAYER) < xBob(BOSS) + SPECIAL_RANGE) ||
        (reg(BOSS_WALK_CHANNEL, 2) != 0 && xBob(PLAYER) < xBob(BOSS) &&
         xBob(PLAYER) > xBob(BOSS) - SPECIAL_RANGE);
    if (close && global(RD) == 0) {
      const int move = m_host.random(2) + 1;
      snapBoss();
      global(RD) = 0;
      global(RP) = BOSS_ENERGY;
      reg(BOSS_WALK_CHANNEL, 7) = word(move);
      reg(BOSS_WALK_CHANNEL, 9) = word(move + 2);
      reg(PLAYER_DAMAGE_CHANNEL, 1) = reg(BOSS_WALK_CHANNEL, 9);
    }
  } else {
    const bool attack = m_host.random(10) == 0;
    const bool touching = bobCol(BOSS) && col(PLAYER);
    if (attack && touching && idle() && clear() && facingPlayer()) {
      snapBoss();
      reg(PLAYER_DAMAGE_CHANNEL, 2) = word(32 + 64 * amosBool(global(RC) == 0));
      global(RD) = 0;
      reg(BOSS_WALK_CHANNEL, 9) = word(m_host.random(1) + 1);
      reg(PLAYER_DAMAGE_CHANNEL, 1) = reg(BOSS_WALK_CHANNEL, 9);
    }
  }

  if (reg(BOSS_DAMAGE_CHANNEL, 2) == 2) {
    reg(BOSS_DAMAGE_CHANNEL, 2) = 0;
    m_machine.start(BOSS_WALK_CHANNEL);
  }
  if (reg(PLAYER_DAMAGE_CHANNEL, 1) == 9) {
    reg(PLAYER_DAMAGE_CHANNEL, 1) = 0;
    m_machine.start(PLAYER_WALK_CHANNEL);
  }

  if (global(RD) == 1 && reg(BOSS_WALK_CHANNEL, 8) == 0) {
    if (bobCol(PLAYER) && col(BOSS) && inFront() &&
        yBob(BOSS) == yBob(PLAYER)) {
      m_machine.freeze(BOSS_WALK_CHANNEL);
      reg(BOSS_DAMAGE_CHANNEL, 1) = word(0x8000 - global(RC));
      reg(BOSS_DAMAGE_CHANNEL, 2) = 1;
      reg(BOSS_DAMAGE_CHANNEL, 0) = 1;
    }
  }
  if (global(RD) == 2 && reg(BOSS_WALK_CHANNEL, 8) == 0) {
    if (bobCol(PLAYER) && col(BOSS) && inFront() &&
        yBob(BOSS) == yBob(PLAYER)) {
      m_machine.freeze(BOSS_WALK_CHANNEL);
      reg(BOSS_DAMAGE_CHANNEL, 3) =
          word(32 + 64 * amosBool(reg(BOSS_DAMAGE_CHANNEL, 1) == 0));
      reg(BOSS_DAMAGE_CHANNEL, 1) = word(0x8000 - global(RC));
      reg(BOSS_DAMAGE_CHANNEL, 2) = 1;
      reg(BOSS_DAMAGE_CHANNEL, 0) = 2;
    }
  }
  if (global(RD) == 4 || global(RD) == 5) {
    const int move = global(RD);
    if (bobCol(PLAYER) && col(BOSS) && reg(BOSS_WALK_CHANNEL, 1) != 1 &&
        global(RB) == yBob(BOSS) && reg(BOSS_DAMAGE_CHANNEL, 2) != 1 &&
        reg(BOSS_WALK_CHANNEL, 3) == 0 && facingBoss()) {
      m_machine.freeze(BOSS_WALK_CHANNEL);
      reg(BOSS_DAMAGE_CHANNEL, 1) = word(0x8000 - global(RC));
      reg(BOSS_DAMAGE_CHANNEL, 3) =
          word(48 + 96 * amosBool(reg(BOSS_DAMAGE_CHANNEL, 1) == 0));
      reg(BOSS_DAMAGE_CHANNEL, 2) = 1;
      reg(BOSS_DAMAGE_CHANNEL, 0) = word(move);
    }
  }
  if (global(RD) == 6 && reg(BOSS_WALK_CHANNEL, 8) == 0) {
    if (bobCol(PLAYER) && reg(BOSS_DAMAGE_CHANNEL, 5) == 0 &&
        global(RC) != reg(BOSS_WALK_CHANNEL, 2) && col(BOSS) &&
        reg(PLAYER_DAMAGE_CHANNEL, 5) == 0 && yBob(PLAYER) == yBob(BOSS) &&
        facingBoss()) {
      m_machine.freeze(BOSS_WALK_CHANNEL);
      m_machine.freeze(PLAYER_WALK_CHANNEL);
      reg(BOSS_DAMAGE_CHANNEL, 1) = word(0x8000 - global(RC));
      m_bobs.setX(BOSS, xBob(PLAYER) + 40 + 80 * amosBool(global(RC) != 0));
      reg(BOSS_DAMAGE_CHANNEL, 6) = word(16 + 32 * amosBool(global(RC) == 0));
      reg(BOSS_DAMAGE_CHANNEL, 2) = 1;
      reg(BOSS_DAMAGE_CHANNEL, 3) =
          word(16 + 32 * amosBool(reg(BOSS_DAMAGE_CHANNEL, 1) == 0));
      reg(PLAYER_DAMAGE_CHANNEL, 5) = 1;
      reg(BOSS_DAMAGE_CHANNEL, 0) = 6;
    }
  }

  if (global(RW) != 0) {
    const int request = global(RW);
    global(RW) = 0;
    playRequest(request);
  } else if (global(RE) != 0) {
    const int request = global(RE);
    global(RE) = 0;
    playRequest(request);
  }

  m_index = BOSS_BLOOD;
  return fightBlood();
}

BossStage::Flow BossStage::fightBlood() {
  for (; m_index <= PLAYER_BLOOD; ++m_index) {
    const int i = m_index;
    if (iBob(i) != SPLAT_IMAGE) {
      continue;
    }
    const int right = m_host.random(8);
    const int left = m_host.random(8);
    const int down = m_host.random(4);
    const int up = m_host.random(4);
    const int image = m_host.random(7) + 2;
    if (pasteStalled(xBob(i) + right - left, yBob(i) + down - up, image)) {
      m_step = Step::FightBloodStamped;
      return Flow::Yield;
    }
  }
  return fightTail();
}

BossStage::Flow BossStage::fightBloodStamped() {
  ++m_index;
  m_step = Step::Fight;
  return fightBlood();
}

BossStage::Flow BossStage::fightTail() {
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
  if (global(RI) == 0) {
    m_panel->drawKills(global(RN));
    global(RC) = word(m_facing);
    return finishStart();
  }
  if (m_escape || global(RG) == -2) {
    gameOver();
    return Flow::Continue;
  }
  sys();
  m_step = Step::Fight;
  return endOfPass();
}

BossStage::Flow BossStage::finishStart() {
  global(RR) = reg(BOSS_WALK_CHANNEL, 2);
  m_machine.destroy(PLAYER_WALK_CHANNEL);
  m_machine.destroy(BOSS_WALK_CHANNEL);
  m_machine.destroy(PLAYER_CLAMP_CHANNEL);
  global(RT) = word(yBob(BOSS) - yBob(PLAYER));
  if (stage() == 1 && m_session.brutality) {
    global(RU) =
        word(xBob(BOSS) - xBob(PLAYER) - 48 - 96 * amosBool(global(RR) != 0));
    global(RS) = word((std::abs(global(RU)) + std::abs(global(RT))) / 2);
    m_machine.create(PLAYER_WALK_CHANNEL, amal::actors::walkToBoss());
    m_machine.startAll();
    return waitFrames(global(RS), Step::FinishWalkedToBoss);
  }
  if (stage() == 2) {
    return liftStart();
  }
  if (stage() == 3) {
    return railingStart();
  }
  return finishWalkOff();
}

BossStage::Flow BossStage::railingStart() {
  for (int image = RAILING_SIT; image <= RAILING_BROKEN; ++image) {
    m_images.noMask(image);
  }
  global(RU) = word(RAILING_X - xBob(BOSS));
  global(RS) = word(RAILING_Y - yBob(BOSS));
  global(RT) = word((std::abs(global(RU)) + std::abs(global(RS))) / 2);
  global(RR) = word(0x8000 * amosBool(global(RB) < 0));
  m_bobs.setImage(PLAYER, word(IDLE_IMAGE + global(RR)));
  m_machine.create(BOSS_WALK_CHANNEL, amal::actors::bossRests());
  m_machine.startAll();
  return waitFrames(REST_WAIT, Step::RailingSpeech);
}

BossStage::Flow BossStage::railingSpeech() {
  m_bobs.set(BOSS_BUBBLE, xBob(BOSS) - 32 - 64 * amosBool(global(RR) != 0),
             yBob(BOSS) - 72, REST_BUBBLE);
  m_machine.create(BOSS_TALK_CHANNEL, amal::actors::bubbleUntilFire());
  m_machine.startAll();
  m_step = Step::RailingWaitFire;
  return Flow::Continue;
}

BossStage::Flow BossStage::railingWaitFire() {
  if (iBob(BOSS_BUBBLE) != HIDDEN_IMAGE) {
    return Flow::Yield;
  }
  reg(BOSS_WALK_CHANNEL, 0) = 1;
  return waitFrames(global(RT), Step::RailingReached);
}

BossStage::Flow BossStage::pasteRailing(int image, Step next) {
  m_step = next;
  return pasteStalled(RAILING_TILE_X, RAILING_TILE_Y, image) ? Flow::Yield
                                                             : Flow::Continue;
}

BossStage::Flow BossStage::liftStart() {
  global(RU) = word(xBob(BOSS) - xBob(PLAYER));
  global(RS) = word((std::abs(global(RU)) + std::abs(global(RT))) / 2);
  m_machine.create(PLAYER_WALK_CHANNEL, amal::actors::walkToBoss());
  m_machine.startAll();
  return waitFrames(global(RS), Step::LiftWalkedToBoss);
}

BossStage::Flow BossStage::liftBoss() {
  m_machine.destroy(PLAYER_WALK_CHANNEL);
  global(RT) = word(global(RQ) * 2);
  m_bobs.set(PLAYER, xBob(BOSS), yBob(BOSS), word(IDLE_IMAGE + global(RR)));
  m_machine.create(BOSS_WALK_CHANNEL, amal::actors::bossThrown());
  m_machine.create(PLAYER_WALK_CHANNEL, amal::actors::victoryLift());
  m_machine.startAll();
  return waitFrames(LIFT_WAIT, Step::LiftRaised);
}

BossStage::Flow BossStage::finishPose() {
  const int facedLeft = amosBool(global(RR) != 0);
  m_machine.destroy(PLAYER_WALK_CHANNEL);
  m_bobs.set(PLAYER, xBob(BOSS) - 64 - 128 * facedLeft, yBob(BOSS),
             word(38 + global(RR)));
  m_machine.create(PLAYER_WALK_CHANNEL, amal::actors::finishingPose());
  m_bobs.set(PLAYER_BUBBLE, xBob(PLAYER) + 8 + 16 * facedLeft,
             yBob(PLAYER) - 32, HIDDEN_IMAGE);
  m_machine.create(PLAYER_TALK_CHANNEL, amal::actors::finishingBlood());
  m_machine.start(PLAYER_WALK_CHANNEL);
  return waitFrames(POSE_WAIT, Step::FinishPosed);
}

BossStage::Flow BossStage::finishBlood() {
  m_bobs.setImage(PLAYER, word(39 + global(RR)));
  m_machine.start(PLAYER_TALK_CHANNEL);
  m_host.setSampleLoop(true);
  m_host.playSample(BOSS_SAMPLE_BANK, FINISHING_SAMPLE, PRIORITY_VOICE);
  m_index = 1;
  return finishStamp();
}

BossStage::Flow BossStage::finishStamp() {
  for (; m_index <= FINISHING_STAMPS; ++m_index) {
    const int left = m_host.random(10);
    const int right = m_host.random(10);
    const int up = m_host.random(10);
    const int down = m_host.random(10);
    const int x = xBob(PLAYER_BUBBLE) - 32 - 40 * amosBool(global(RR) == 0) -
                  left + right;
    const int y = yBob(PLAYER) - 4 - up + down;
    if (pasteStalled(x, y, word(90 - global(RR)))) {
      m_step = Step::FinishStamped;
      return Flow::Yield;
    }
  }
  m_host.setSampleLoop(false);
  m_machine.create(PLAYER_WALK_CHANNEL, amal::actors::finishingPoseBack());
  m_machine.start(PLAYER_WALK_CHANNEL);
  m_machine.destroy(PLAYER_TALK_CHANNEL);
  m_bobs.setImage(PLAYER_BUBBLE, HIDDEN_IMAGE);
  return waitFrames(POSE_WAIT, Step::FinishPosedBack);
}

BossStage::Flow BossStage::finishWalkOff() {
  m_host.playSample(PLAYER_SAMPLE_BANK, WALK_OFF_SAMPLE, PRIORITY_VOICE);
  global(RT) = word(WALK_OFF_DISTANCE * amosBool(global(RC) != 0) -
                    WALK_OFF_DISTANCE * amosBool(global(RC) == 0));
  global(RU) = word(std::abs(global(RT)));
  m_machine.create(PLAYER_WALK_CHANNEL, amal::actors::walkOff());
  m_machine.startAll();
  return waitFrames(global(RU), Step::FinishWalkedOff);
}

BossStage::Flow BossStage::finishCleanUp() {
  if (global(RG) == -2 || m_escape) {
    gameOver();
    return Flow::Continue;
  }
  m_machine.destroyAll();
  m_bobs.offAll();
  if (stage() == 3) {
    m_buffer.autoback([](IndexedSurface &surface) { surface.fill(0); });
    m_session.bossExit.emplace(BossExit{m_buffer, m_palette, m_screenDisplay.y,
                                        m_screenOffsetX, m_panel->surface(),
                                        panelDisplayY(stageLayout(m_options)),
                                        m_options.tallScreen});
    m_outcome = Outcome::BossDefeated;
    m_step = Step::Finished;
    return Flow::Yield;
  }
  m_step = Step::Cleared;
  autoback([](IndexedSurface &surface) { surface.fill(0); });
  return Flow::Yield;
}

void BossStage::scrollStep() {
  const int dx = stage() == 2 ? 8 : -8;
  autoback([dx](IndexedSurface &surface) {
    surface.copy(surface, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, dx, 0);
  });
}

void BossStage::gameOver() {
  m_session.stageReached = global(RO);
  global(RO) = -1;
  if (m_escape) {
    global(RN) = 0;
    closePlayScreen();
    return;
  }
  m_resumeFrame = m_frame + GAME_OVER_WAIT;
  m_step = Step::GameOverWait;
}

void BossStage::closePlayScreen() {
  m_resumeFrame = m_frame + effects::SCREEN_CLOSE_SHOWN_VBLS;
  m_step = Step::GameOverScreenGone;
}

void BossStage::sys() {
  const SystemKey key = m_pendingKey;
  m_pendingKey = SystemKey::None;
  switch (key) {
  case SystemKey::MusicOff:
    m_host.setMusicVolume(0);
    m_options.music = false;
    break;
  case SystemKey::MusicOn:
    m_options.music = true;
    m_host.setMusicVolume(MUSIC_VOLUME);
    break;
  case SystemKey::Pal:
  case SystemKey::Ntsc:
    switchStandard(m_options, m_screenDisplay, key == SystemKey::Ntsc);
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

void BossStage::runBasic(const StreetInput &input) {
  Flow flow = Flow::Continue;
  while (flow == Flow::Continue && m_frame >= m_resumeFrame) {
    switch (m_step) {
    case Step::Init:
      flow = init();
      break;
    case Step::BossMusic:
      flow = bossMusic();
      break;
    case Step::BossLoaded:
      bossLoaded();
      setUp();
      m_step = Step::Approach;
      break;
    case Step::Loading:
      if (m_loading.advance(m_panel.get())) {
        m_step = m_afterLoading;
      } else {
        flow = Flow::Yield;
      }
      break;
    case Step::Approach:
      flow = approachTop(input);
      break;
    case Step::ApproachUnpacked:
      flow = approachScroll();
      break;
    case Step::ApproachScrolled:
      flow = approachScrolled();
      break;
    case Step::ChildPasted:
      m_index = 1;
      m_bobs.setImage(BOSS, CHILD_STANDS);
      flow = waitFrames(CHILD_WAIT, Step::ChildHit);
      break;
    case Step::ChildHit:
      m_bobs.setImage(BOSS, CHILD_PUNCH);
      flow = waitFrames(CHILD_WAIT, Step::ChildRaised);
      break;
    case Step::ChildRaised:
      flow = childRaised();
      break;
    case Step::ChildCried:
      m_bobs.setImage(BOSS, CHILD_PUNCH);
      beginTalk();
      m_step = Step::Dialogue;
      break;
    case Step::Dialogue:
      flow = dialogue();
      break;
    case Step::Fight:
      flow = fightTop();
      break;
    case Step::FightBloodStamped:
      flow = fightBloodStamped();
      break;
    case Step::FinishWalkedToBoss:
      flow = finishPose();
      break;
    case Step::FinishPosed:
      flow = finishBlood();
      break;
    case Step::FinishStamped:
      ++m_index;
      flow = finishStamp();
      break;
    case Step::FinishPosedBack:
      flow = finishWalkOff();
      break;
    case Step::FinishWalkedOff:
      flow = finishCleanUp();
      break;
    case Step::LiftWalkedToBoss:
      flow = liftBoss();
      break;
    case Step::LiftRaised:
      m_host.playSample(BOSS_SAMPLE_BANK, BOSS_THROWN_SAMPLE, PRIORITY_VOICE);
      flow = waitFrames(THROW_WAIT, Step::LiftThrown);
      break;
    case Step::LiftThrown:
      m_host.playSample(PLAYER_SAMPLE_BANK, VICTORY_SAMPLE, PRIORITY_VOICE);
      flow = waitFrames(SHOUT_WAIT, Step::LiftDone);
      break;
    case Step::LiftDone:
      flow = finishWalkOff();
      break;
    case Step::RailingSpeech:
      flow = railingSpeech();
      break;
    case Step::RailingWaitFire:
      flow = railingWaitFire();
      break;
    case Step::RailingReached:
      m_machine.destroy(BOSS_WALK_CHANNEL);
      m_bobs.off(BOSS);
      flow = waitFrames(1, Step::RailingSat);
      break;
    case Step::RailingSat:
      flow = pasteRailing(RAILING_SIT, Step::RailingSitting);
      break;
    case Step::RailingSitting:
      flow = waitFrames(SIT_WAIT, Step::RailingCurse);
      break;
    case Step::RailingCurse:
      m_host.playSample(BOSS_SAMPLE_BANK, CURSE_SAMPLE, PRIORITY_VOICE);
      m_bobs.set(BOSS_BUBBLE, CURSE_BUBBLE_X, CURSE_BUBBLE_Y, CURSE_BUBBLE);
      m_index = RAILING_SIT + 1;
      flow = waitFrames(1, Step::RailingFall);
      break;
    case Step::RailingFall:
      flow = pasteRailing(m_index, Step::RailingFell);
      break;
    case Step::RailingFell:
      flow = waitFrames(FALL_WAIT, Step::RailingFallNext);
      break;
    case Step::RailingFallNext:
      flow = ++m_index <= RAILING_BROKEN
                 ? waitFrames(1, Step::RailingFall)
                 : waitFrames(FALLEN_WAIT, Step::RailingQuiet);
      break;
    case Step::RailingQuiet:
      m_bobs.setImage(BOSS_BUBBLE, HIDDEN_IMAGE);
      flow = waitFrames(VICTORY_POSE_DELAY, Step::RailingPose);
      break;
    case Step::RailingPose:
      m_bobs.setImage(PLAYER, POSE_IMAGE);
      flow = waitFrames(POSE_HOLD, Step::RailingGrin);
      break;
    case Step::RailingGrin:
      m_bobs.setImage(PLAYER, GRIN_IMAGE);
      m_host.playSample(PLAYER_SAMPLE_BANK, GRIN_SAMPLE, PRIORITY_VOICE);
      flow = waitFrames(GRIN_HOLD, Step::RailingGrinned);
      break;
    case Step::RailingGrinned:
      m_bobs.setImage(PLAYER, POSE_IMAGE);
      flow = waitFrames(POSE_HOLD, Step::RailingDone);
      break;
    case Step::RailingDone:
      flow = finishWalkOff();
      break;
    case Step::Cleared:
      m_outcome = Outcome::BossDefeated;
      m_step = Step::Finished;
      flow = Flow::Yield;
      break;
    case Step::GameOverWait:
      closePlayScreen();
      flow = Flow::Yield;
      break;
    case Step::GameOverScreenGone:
      m_screenShown = false;
      m_resumeFrame = m_frame + effects::SCREEN_CLOSE_HIDDEN_VBLS;
      m_step = Step::GameOverPanelClose;
      flow = Flow::Yield;
      break;
    case Step::GameOverPanelClose:
      m_resumeFrame = m_frame + effects::SCREEN_CLOSE_SHOWN_VBLS;
      m_step = Step::GameOverPanelGone;
      flow = Flow::Yield;
      break;
    case Step::GameOverPanelGone:
      m_panelShown = false;
      m_resumeFrame = m_frame + effects::SCREEN_CLOSE_HIDDEN_VBLS;
      m_step = Step::GameOverClosed;
      flow = Flow::Yield;
      break;
    case Step::GameOverClosed:
      m_outcome = m_escape ? Outcome::Quit : Outcome::GameOver;
      m_step = Step::Finished;
      flow = Flow::Yield;
      break;
    case Step::Finished:
      flow = Flow::Yield;
      break;
    }
  }
}

} // namespace openfranko::src::engine::street
