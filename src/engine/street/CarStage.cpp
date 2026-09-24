#include "CarStage.h"

#include "../amal/Actors.h"
#include "SystemText.h"

#include <cstdlib>
#include <string>

namespace openfranko::src::engine::street {
namespace {

using amal::actors::amosBool;

constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RN = 13;
constexpr int RO = 14;
constexpr int RT = 19;
constexpr int RU = 20;

constexpr int AUTOBACK_VBLS = 3;
constexpr int SCREEN_OPEN_VBLS = 1;
constexpr int SCREEN_CLOSE_VBLS = 2;
constexpr int GAME_OVER_WAIT = 200;
constexpr int IGNITION_WAIT = 30;
constexpr int FULL_ENERGY = 64;

constexpr int PASSWORD_X = 124;
constexpr int PASSWORD_BASELINE = 111;
constexpr uint8_t PASSWORD_INK = 9;
constexpr uint8_t PASSWORD_PAPER = 0;
constexpr int PASSWORD_SHIFT = 8;

constexpr int CAR_SET = 150;
constexpr int CAR_SAMPLE_BANK = 2;
constexpr int PEDESTRIAN_IMAGES = 7;
constexpr int ROAD_PICTURE = 906;

constexpr int STRIP_WIDTH = 2 * CarStage::ROAD_WIDTH;
constexpr int VISIBLE_WIDTH = 304;
constexpr int BAND_END = 366;

constexpr int START_X = 96;
constexpr int FORWARD_X = 120;
constexpr int START_Y = 168;
constexpr int TOP_KERB = 124;
constexpr int BOTTOM_KERB = 198;
constexpr int TRAIL = 5020;
constexpr int TOP_SPEED = 12;
constexpr int PULL = 4;
constexpr int IGNITION_CYCLE = 50;
constexpr int HORN_CYCLE = 2;
constexpr int CLOCK_CYCLE = 4;
constexpr int KERB_DAMAGE = 8;
constexpr int PEDESTRIAN_ENERGY = 14;
constexpr int KIND_THAT_HURTS = 18;
constexpr int DRIVE_OFF_X = 1200;

constexpr int BUSH_A = 3;
constexpr int BUSH_B = 4;
constexpr int BUSH_A_IMAGE = 7;
constexpr int BUSH_B_IMAGE = 8;
constexpr int BUSH_Y = 222;
constexpr int BUSH_START = 0;
constexpr int BUSH_SPACING = 184;
constexpr int BUSH_LEFT = -40;
constexpr int BUSH_RIGHT = 367;
constexpr int PARKED_X = -1000;
constexpr int PARKED_IMAGE = 14;

constexpr int FIRST_KIND = 9;
constexpr int KIND_SPACING = 5;
constexpr int SPAWN_X = 340;
constexpr int SPAWN_TOP = 93;
constexpr int SPAWN_STEP = 4;
constexpr int HIT_REACH_ABOVE = 32;
constexpr int HIT_REACH_BELOW = 8;
constexpr int SQUASH_REGISTER = 4;
constexpr int KIND_REGISTER = 2;

constexpr int HIT_SAMPLE = 1;
constexpr int HORN_SAMPLE = 2;
constexpr int SQUASH_SAMPLE = 3;
constexpr int IGNITION_SAMPLE = 4;
constexpr int ENGINE_SAMPLE = 5;
constexpr int BRAKE_SAMPLE = 6;
constexpr int KERB_SAMPLE = 7;
constexpr int ENGINE_VOICE = 8;
constexpr int ENGINE_PITCH = 5000;
constexpr int ENGINE_PITCH_STEP = 200;

constexpr int16_t JOY_UP = 1;
constexpr int16_t JOY_DOWN = 2;
constexpr int16_t JOY_LEFT = 4;
constexpr int16_t JOY_RIGHT = 8;
constexpr int16_t JOY_FIRE = 16;

int16_t word(int value) { return static_cast<int16_t>(value); }

void addWrap(int &value, int step, int low, int high) {
  value += step;
  if (value > high) {
    value = low;
  } else if (value < low) {
    value = high;
  }
}

std::string passwordFor(int stage) {
  const std::string encoded = stage == 2 ? "SWLB(LZbM" : "SWLB(KMV\\";
  std::string decoded;
  for (char character : encoded) {
    decoded += static_cast<char>(character - PASSWORD_SHIFT);
  }
  return decoded;
}

} // namespace

CarStage::CarStage(StreetHost &host, GameSession &session,
                   effects::GameOptions &options)
    : m_host(host), m_session(session), m_machine(session.registers),
      m_screen(SCREEN_WIDTH, SCREEN_HEIGHT),
      m_display(SCREEN_WIDTH, SCREEN_HEIGHT), m_road(0, 0), m_strip(0, 0),
      m_panel(std::make_unique<StatusPanel>(
          host.loadPanelPicture(StreetStage::LOADING_STRIP),
          host.loadPanelPicture(StreetStage::PANEL_ARTWORK))),
      m_screenDisplay{DISPLAY_X, DISPLAY_TOP, 0},
      m_palette(levelPalette(options.mono)), m_panelPalette(panelPalette()),
      m_screenOffsetX(stage() == 2 ? 16 : 0) {
  m_panel->score(stats());
}

void CarStage::advance(const StreetInput &input) {
  if (m_step == Step::Finished) {
    return;
  }
  ++m_frame;
  if (input.key != SystemKey::None) {
    m_pendingKey = input.key;
  }
  m_machine.tick();
  runBasic(input);
  redraw();
}

void CarStage::compose(std::vector<uint32_t> &frame) const {
  composeFrame(frame, &m_display, m_palette, m_screenDisplay, m_screenOffsetX,
               m_panel.get(), m_panelPalette);
}

CarStage::Outcome CarStage::outcome() const { return m_outcome; }

const BobLayer &CarStage::bobs() const { return m_bobs; }

const IndexedSurface &CarStage::screen() const { return m_screen; }

const IndexedSurface &CarStage::display() const { return m_display; }

const StatusPanel *CarStage::panel() const { return m_panel.get(); }

amal::Machine &CarStage::machine() { return m_machine; }

bool CarStage::isShowingPassword() const {
  return m_step == Step::Password || m_step == Step::PasswordText ||
         m_step == Step::Kliker;
}

bool CarStage::isDriving() const {
  return m_step == Step::DriveTop || m_step == Step::DriveIgnited ||
         m_step == Step::DriveBottom;
}

int CarStage::distance() const { return m_distance; }

int CarStage::speed() const { return m_speed; }

int CarStage::carX() const { return m_x; }

int CarStage::carY() const { return m_y; }

bool CarStage::isEngineOn() const { return m_ignition != 0; }

int16_t &CarStage::global(int index) { return m_machine.globalRegister(index); }

int CarStage::stage() const { return m_session.registers[RO]; }

StatusPanel::Stats CarStage::stats() const {
  const amal::Registers &registers = m_session.registers;
  return {registers[RF], registers[RO], registers[RN], registers[RG]};
}

CarStage::Flow CarStage::wait(int frames, Step next) {
  m_step = next;
  m_resumeFrame = m_frame + frames;
  return Flow::Yield;
}

void CarStage::play(int voices, int sample) {
  m_host.playSample(CAR_SAMPLE_BANK, sample, voices);
}

void CarStage::loseEnergy(int amount) {
  global(RF) = word(global(RF) - amount);
  if (global(RF) < 0) {
    global(RF) = word(FULL_ENERGY + global(RF));
    global(RG) = word(global(RG) - 1);
    m_panel->score(stats());
  } else {
    m_panel->loseEnergy(global(RF));
  }
}

void CarStage::gainEnergy(int amount) {
  global(RF) = word(global(RF) + amount);
  if (global(RF) > FULL_ENERGY) {
    global(RF) = word(global(RF) - FULL_ENERGY);
    global(RG) = word(global(RG) + 1);
    m_panel->score(stats());
  } else {
    m_panel->gainEnergy(global(RF));
  }
}

void CarStage::clearScreen() { m_screen.fill(0); }

void CarStage::password() {
  m_screenOffsetX = 0;
  drawSystemText(m_screen, PASSWORD_X, PASSWORD_BASELINE, passwordFor(stage()),
                 PASSWORD_INK, PASSWORD_PAPER);
  m_waited = 0;
}

void CarStage::era() {
  m_host.stopMusic();
  m_images.clear();
  m_loading.queue([this] {
    m_images.load(1, m_host.loadSpriteSet(CAR_SET, CAR_SAMPLE_BANK));
  });
  m_loading.queue([this] {
    m_images.load(PEDESTRIAN_IMAGES,
                  m_host.loadSpriteSet(CAR_SET - stage(), 0));
  });
  m_loading.queue(
      [this] { m_backdrop = m_host.loadPicture(ROAD_PICTURE + stage()); });
  m_afterLoading = Step::Loaded;
  m_step = Step::Loading;
}

void CarStage::openRoad() {
  m_road.fill(0);
  m_road.unpack(m_backdrop, 0, 0);
  m_backdrop = Picture{};
  m_strip = IndexedSurface(STRIP_WIDTH, SCREEN_HEIGHT);
}

void CarStage::openStrip() {
  m_strip.fill(0);
  m_strip.copy(m_road, 0, 0, ROAD_WIDTH, SCREEN_HEIGHT, 0, 0);
  m_strip.copy(m_road, 0, 0, ROAD_WIDTH, SCREEN_HEIGHT, ROAD_WIDTH, 0);
  m_road = IndexedSurface(0, 0);
}

void CarStage::startDrive() {
  m_screen.copy(m_strip, 0, 0, VISIBLE_WIDTH, SCREEN_HEIGHT, 0, 0);
  for (int channel = 1; channel <= PEDESTRIANS; ++channel) {
    m_machine.bind(channel, &m_bobs.object(FIRST_PEDESTRIAN + channel - 1));
  }
  m_machine.bind(CAR_CHANNEL, &m_bobs.object(CAR));
  m_x = START_X;
  m_y = START_Y;
  m_distance = DISTANCE;
  m_trail = TRAIL;
  m_topSpeed = TOP_SPEED;
  m_pull = PULL;
  m_speed = 0;
  m_horn = 0;
  m_accel = 0;
  m_bush1 = BUSH_START;
  m_bush2 = BUSH_SPACING;
  m_trackBand = 10;
  m_pavementBand = 5;
  m_manualBobs = true;
  m_bobs.set(CAR, m_x, m_y, 1);
  for (int bob = FIRST_PEDESTRIAN; bob < FIRST_PEDESTRIAN + PEDESTRIANS;
       ++bob) {
    m_bobs.set(bob, PARKED_X, 0, PARKED_IMAGE);
  }
}

CarStage::Flow CarStage::driveTop(const StreetInput &input) {
  if (m_distance > 0 && (input.joystick & JOY_FIRE)) {
    if (m_speed != 0) {
      addWrap(m_horn, 1, 0, HORN_CYCLE);
      if (m_horn == 0) {
        play(1, HORN_SAMPLE);
      }
    } else if (m_x == START_X) {
      addWrap(m_ignition, 1, 1, IGNITION_CYCLE);
      if (m_ignition == 1) {
        play(1, IGNITION_SAMPLE);
        return wait(IGNITION_WAIT, Step::DriveIgnited);
      }
    }
  }
  return driveInput(input);
}

CarStage::Flow CarStage::driveInput(const StreetInput &input) {
  if (m_distance > 0) {
    const int16_t joystick = input.joystick;
    if ((joystick & JOY_UP) && m_speed != 0) {
      m_steer = 2;
      m_y += -m_speed / 2 + 1;
    }
    if ((joystick & JOY_DOWN) && m_speed != 0) {
      m_steer = 4;
      m_y += m_speed / 2 - 1;
    }
    if (m_y < TOP_KERB) {
      hitKerb(TOP_KERB);
    }
    if (m_y > BOTTOM_KERB) {
      hitKerb(BOTTOM_KERB);
    }
    addWrap(m_accel, 1, 1, m_pull);
    if ((joystick & JOY_RIGHT) && m_ignition != 0) {
      if (m_x < FORWARD_X) {
        ++m_x;
      }
      if (m_accel == m_pull) {
        ++m_speed;
        if (m_speed > m_topSpeed) {
          m_speed = m_topSpeed;
        }
      }
    } else {
      if (m_x > START_X) {
        --m_x;
      }
      if (m_accel == m_pull) {
        --m_speed;
        if (m_speed < 0) {
          m_speed = 0;
          m_ignition = 0;
        }
      }
    }
    if ((joystick & JOY_LEFT) && m_ignition != 0 && m_trail > m_distance) {
      addWrap(m_accel, 1, 1, m_pull / 4);
      if (m_accel == 1) {
        --m_speed;
        if (m_speed < -1) {
          m_speed = -1;
        }
        if (m_speed > 0) {
          play(1, BRAKE_SAMPLE);
        }
      }
    }
    global(RT) = word(8 * m_speed);
    global(RU) = word(5 * m_speed);
    spawnPedestrians();
  }
  return driveScenery();
}

void CarStage::hitKerb(int kerb) {
  m_y = kerb;
  m_speed = -1 - 2 * amosBool(m_speed < 0);
  play(1, KERB_SAMPLE);
  loseEnergy(KERB_DAMAGE);
}

void CarStage::spawnPedestrians() {
  for (int channel = 1; channel <= PEDESTRIANS; ++channel) {
    const bool idle = !m_machine.isRunning(channel);
    const bool lucky = m_host.random(5) > 3;
    if (!idle || !lucky) {
      continue;
    }
    m_hit[static_cast<std::size_t>(channel)] = false;
    const int kind = FIRST_KIND + KIND_SPACING * m_host.random(3);
    const int x = m_host.random(200) + SPAWN_X;
    const int y = SPAWN_TOP + m_host.random(20) * SPAWN_STEP;
    m_bobs.set(FIRST_PEDESTRIAN + channel - 1, x, y, kind);
    m_machine.create(channel, amal::actors::pedestrian(kind));
    m_machine.startAll();
  }
}

CarStage::Flow CarStage::driveScenery() {
  addWrap(m_trackBand, m_speed * 2, 0, BAND_END);
  addWrap(m_pavementBand, m_speed * 4, 0, BAND_END);
  addWrap(m_roadBand, m_speed * 3, 0, BAND_END);
  addWrap(m_fenceBand, m_speed, 0, BAND_END);
  m_screen.copy(m_strip, m_trackBand, 93, VISIBLE_WIDTH + m_trackBand, 115, 0,
                93);
  m_screen.copy(m_strip, m_pavementBand, 202, VISIBLE_WIDTH + m_pavementBand,
                222, 0, 202);
  m_screen.copy(m_strip, m_fenceBand, 0, VISIBLE_WIDTH + m_fenceBand, 94, 0, 0);
  m_screen.copy(m_strip, m_roadBand, 95, VISIBLE_WIDTH + m_roadBand, 201, 0,
                95);
  if (m_x != START_X || m_speed != 0) {
    addWrap(m_clock, 1, 1, CLOCK_CYCLE);
  }
  if (m_distance > 0) {
    if (m_clock == 1) {
      m_bobs.set(CAR, m_x, m_y, 1 + m_steer);
    }
    if (m_clock == 3) {
      m_bobs.set(CAR, m_x, m_y, 2 + m_steer);
    }
  }
  m_steer = 0;
  addWrap(m_bush1, -m_speed * 8, BUSH_LEFT, BUSH_RIGHT);
  addWrap(m_bush2, -m_speed * 8, BUSH_LEFT, BUSH_RIGHT);
  m_bobs.set(BUSH_A, m_bush1, BUSH_Y, BUSH_A_IMAGE);
  m_bobs.set(BUSH_B, m_bush2, BUSH_Y, BUSH_B_IMAGE);
  bobDraw();
  return wait(1, Step::DriveBottom);
}

CarStage::Flow CarStage::driveBottom() {
  runOver();
  if (m_x != START_X || m_speed != 0) {
    addWrap(m_engineBeat, 1, 0, 1);
    if (m_engineBeat == 0) {
      m_host.playSampleAt(CAR_SAMPLE_BANK, ENGINE_SAMPLE, ENGINE_VOICE,
                          ENGINE_PITCH + std::abs(m_speed * ENGINE_PITCH_STEP));
    }
  }
  if (m_distance > 0) {
    m_distance -= m_speed;
    if (m_speed > 0 && m_trail - m_distance > 20) {
      m_trail -= std::abs(m_speed);
    }
    if (m_distance == 0) {
      m_distance = -1;
    }
  }
  if (m_distance < 0) {
    m_x = DRIVE_OFF_X;
    m_distance = 0;
    m_machine.create(CAR_CHANNEL, amal::actors::carDriveOff());
    m_machine.start(CAR_CHANNEL);
  }
  if ((m_distance == 0 && !m_machine.isRunning(CAR_CHANNEL)) ||
      global(RG) < 0 || m_escape) {
    m_manualBobs = false;
    return wait(SCREEN_CLOSE_VBLS, Step::StripClosed);
  }
  sys();
  m_step = Step::DriveTop;
  return Flow::Continue;
}

void CarStage::runOver() {
  for (int channel = 1; channel <= PEDESTRIANS; ++channel) {
    const int bob = FIRST_PEDESTRIAN + channel - 1;
    const bool touched = m_bobs.collide(CAR, m_images) && m_bobs.collided(bob);
    const int y = m_bobs.y(bob);
    const int carY = m_bobs.y(CAR);
    if (!touched || y <= carY - HIT_REACH_ABOVE ||
        y >= carY + HIT_REACH_BELOW ||
        m_hit[static_cast<std::size_t>(channel)]) {
      continue;
    }
    m_speed /= 4;
    m_hit[static_cast<std::size_t>(channel)] = true;
    play(2, SQUASH_SAMPLE);
    play(1, HIT_SAMPLE);
    m_machine.channelRegister(channel, SQUASH_REGISTER) = 1;
    if (m_machine.channelRegister(channel, KIND_REGISTER) > KIND_THAT_HURTS) {
      loseEnergy(PEDESTRIAN_ENERGY);
    } else {
      gainEnergy(PEDESTRIAN_ENERGY);
    }
  }
}

CarStage::Flow CarStage::leave() {
  m_strip = IndexedSurface(0, 0);
  if (global(RG) < 0 || m_escape) {
    gameOver();
    return Flow::Yield;
  }
  m_machine.destroyAll();
  m_bobs.offAll();
  clearScreen();
  return wait(AUTOBACK_VBLS, Step::Cleared);
}

void CarStage::gameOver() {
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

void CarStage::sys() {
  const SystemKey key = m_pendingKey;
  m_pendingKey = SystemKey::None;
  if (key == SystemKey::Escape) {
    global(RN) = 0;
    m_escape = true;
    m_machine.freezeAll();
  }
}

void CarStage::bobDraw() {
  m_display = m_screen;
  m_bobs.draw(m_display, m_images);
}

void CarStage::runBasic(const StreetInput &input) {
  Flow flow = Flow::Continue;
  while (flow == Flow::Continue && m_frame >= m_resumeFrame) {
    switch (m_step) {
    case Step::Password:
      clearScreen();
      flow = wait(AUTOBACK_VBLS, Step::PasswordText);
      break;
    case Step::PasswordText:
      password();
      flow = wait(AUTOBACK_VBLS, Step::Kliker);
      break;
    case Step::Kliker:
      ++m_waited;
      if (m_waited > PASSWORD_WAIT || input.joystick > 0) {
        m_step = Step::Era;
      } else {
        flow = wait(1, Step::Kliker);
      }
      break;
    case Step::Era:
      era();
      break;
    case Step::Loading:
      if (m_loading.advance(m_panel.get())) {
        m_step = m_afterLoading;
      } else {
        flow = Flow::Yield;
      }
      break;
    case Step::Loaded:
      m_panel->score(stats());
      m_road = IndexedSurface(ROAD_WIDTH, SCREEN_HEIGHT);
      flow = wait(SCREEN_OPEN_VBLS, Step::RoadOpened);
      break;
    case Step::RoadOpened:
      openRoad();
      flow = wait(SCREEN_OPEN_VBLS, Step::StripOpened);
      break;
    case Step::StripOpened:
      openStrip();
      flow = wait(SCREEN_CLOSE_VBLS, Step::RoadClosed);
      break;
    case Step::RoadClosed:
      startDrive();
      m_step = Step::DriveTop;
      break;
    case Step::DriveTop:
      flow = driveTop(input);
      break;
    case Step::DriveIgnited:
      flow = driveInput(input);
      break;
    case Step::DriveBottom:
      flow = driveBottom();
      break;
    case Step::StripClosed:
      flow = leave();
      break;
    case Step::Cleared:
      m_outcome = Outcome::DriveFinished;
      m_step = Step::Finished;
      flow = Flow::Yield;
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

void CarStage::redraw() {
  if (m_manualBobs) {
    return;
  }
  m_display = m_screen;
  m_bobs.draw(m_display, m_images);
}

} // namespace openfranko::src::engine::street
