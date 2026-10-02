#include "CarStage.h"

#include "../../../systems/input/ControllerSystem.h"
#include "../../AmigaDisplay.h"
#include "../actors/Actors.h"
#include "../actors/compiled/CompiledActors.h"
#include "../core/SystemText.h"

#include <cstdlib>
#include <string>
#include <utility>

namespace openfranko::src::engine::street::scenes {
namespace {

constexpr int IGNITION_WAIT = 30;

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

CarStage::CarStage(StreetHost &host, session::GameSession &session,
                   GameOptions &options)
    : Stage(host, session, options), m_road(0, 0), m_strip(0, 0) {
  openBlankScreens();
  m_holdsWhileClosing = true;
  openPanel();
  m_screenOffsetX = stage() == 2 ? 16 : 0;
  m_panel->score(stats());
}

bool CarStage::isShowingPassword() const {
  return m_step == Step::Password || m_step == Step::PasswordText ||
         m_step == Step::Kliker;
}

int CarStage::passes() const { return m_passes; }

bool CarStage::isDriving() const {
  return m_step == Step::DriveTop || m_step == Step::DriveIgnited ||
         m_step == Step::DriveScenery || m_step == Step::DriveBottom;
}

int CarStage::distance() const { return m_distance; }

int CarStage::speed() const { return m_speed; }

int CarStage::carX() const { return m_x; }

int CarStage::carY() const { return m_y; }

bool CarStage::isEngineOn() const { return m_ignition != 0; }

CarStage::Flow CarStage::wait(int frames, Step next) {
  m_step = next;
  m_resumeFrame = m_frame + frames;
  return Flow::Yield;
}

CarStage::Flow CarStage::hold(int frames, Step next) {
  m_step = next;
  return Stage::hold(frames);
}

CarStage::Flow CarStage::autoback(core::DoubleBuffer::Op op, Step next) {
  Stage::autoback(std::move(op));
  m_step = next;
  return Flow::Yield;
}

void CarStage::play(int voices, int sample) {
  m_host.playSample(CAR_SAMPLE_BANK, sample, voices);
}

void CarStage::loseEnergy(int amount) {
  global(amal::RF) = word(global(amal::RF) - amount);
  if (global(amal::RF) < 0) {
    global(amal::RF) = word(session::FULL_ENERGY + global(amal::RF));
    global(amal::RG) = word(global(amal::RG) - 1);
    m_panel->score(stats());
  } else {
    m_panel->loseEnergy(global(amal::RF));
  }
}

void CarStage::gainEnergy(int amount) {
  global(amal::RF) = word(global(amal::RF) + amount);
  if (global(amal::RF) > session::FULL_ENERGY) {
    global(amal::RF) = word(global(amal::RF) - session::FULL_ENERGY);
    global(amal::RG) = word(global(amal::RG) + 1);
    m_panel->score(stats());
  } else {
    m_panel->gainEnergy(global(amal::RF));
  }
}

void CarStage::clearScreen() {
  autoback([](core::IndexedSurface &surface) { surface.fill(0); },
           m_session.version == GameVersion::V12 ? Step::HideForLoading
                                                 : Step::PasswordText);
}

void CarStage::hideForLoading() {
  m_screenOffsetX = 0;
  hideScreen();
  m_step = Step::Era;
}

void CarStage::password() {
  m_screenOffsetX = 0;
  m_waited = 0;
  m_session.textBuffer = passwordFor(stage());
  autoback(
      [text = m_session.textBuffer](core::IndexedSurface &surface) {
        core::drawSystemText(surface, PASSWORD_X, PASSWORD_BASELINE, text,
                             PASSWORD_INK, PASSWORD_PAPER);
      },
      Step::Kliker);
}

void CarStage::era() {
  m_host.stopMusic();
  m_musicLoaded = false;
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
  m_backdrop = core::Picture{};
  m_strip = core::IndexedSurface(STRIP_WIDTH, SCREEN_HEIGHT);
}

void CarStage::openStrip() {
  m_strip.fill(0);
  m_strip.copy(m_road, 0, 0, ROAD_WIDTH, SCREEN_HEIGHT, 0, 0);
  m_strip.copy(m_road, 0, 0, ROAD_WIDTH, SCREEN_HEIGHT, ROAD_WIDTH, 0);
  m_road = core::IndexedSurface(0, 0);
}

void CarStage::startDrive() {
  m_screenShown = true;
  m_screen.copy(m_strip, 0, 0, VISIBLE_WIDTH, SCREEN_HEIGHT, 0, 0);
  m_buffer.logic().copy(m_strip, 0, 0, VISIBLE_WIDTH, SCREEN_HEIGHT, 0, 0);
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
  const session::DriveCarryOver &left = m_session.lastDrive;
  m_ignition = left.ignition;
  m_roadBand = left.roadBand;
  m_fenceBand = left.fenceBand;
  m_clock = left.clock;
  m_engineBeat = left.engineBeat;
  m_buffer.setUpdates(false);
  m_bobs.set(CAR, m_x, m_y, 1);
  for (int bob = FIRST_PEDESTRIAN; bob < FIRST_PEDESTRIAN + PEDESTRIANS;
       ++bob) {
    m_bobs.set(bob, PARKED_X, 0, PARKED_IMAGE);
  }
  m_buffer.swap();
}

CarStage::Flow CarStage::driveTop(const StreetInput &input) {
  ++m_passes;
  if (m_distance > 0 && (input.joystick & systems::input::JOY_FIRE)) {
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
    if ((joystick & systems::input::JOY_UP) && m_speed != 0) {
      m_steer = 2;
      m_y += -m_speed / 2 + 1;
    }
    if ((joystick & systems::input::JOY_DOWN) && m_speed != 0) {
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
    if ((joystick & systems::input::JOY_RIGHT) && m_ignition != 0) {
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
    if ((joystick & systems::input::JOY_LEFT) && m_ignition != 0 &&
        m_trail > m_distance) {
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
    global(amal::RT) = word(8 * m_speed);
    global(amal::RU) = word(5 * m_speed);
    spawnPedestrians();
  }
  const int frames = nextPassFrames();
  if (frames > 1) {
    return wait(frames - 1, Step::DriveScenery);
  }
  return driveScenery();
}

int CarStage::nextPassFrames() {
  m_passTime += m_options.ntsc ? systems::graphics::NTSC_HERTZ
                               : systems::graphics::PAL_HERTZ;
  const int frames = m_passTime / PASSES_PER_SECOND;
  m_passTime %= PASSES_PER_SECOND;
  return frames;
}

void CarStage::hitKerb(int kerb) {
  m_y = kerb;
  m_speed = -1 - 2 * actors::amosBool(m_speed < 0);
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
    m_machine.create(channel, actors::compiled::pedestrian(kind));
    m_machine.startAll();
  }
}

CarStage::Flow CarStage::driveScenery() {
  addWrap(m_trackBand, m_speed * 2, 0, BAND_END);
  addWrap(m_pavementBand, m_speed * 4, 0, BAND_END);
  addWrap(m_roadBand, m_speed * 3, 0, BAND_END);
  addWrap(m_fenceBand, m_speed, 0, BAND_END);
  for (core::IndexedSurface *target : {&m_screen, &m_buffer.logic()}) {
    target->copy(m_strip, m_trackBand, 93, VISIBLE_WIDTH + m_trackBand, 115, 0,
                 93);
    target->copy(m_strip, m_pavementBand, 202, VISIBLE_WIDTH + m_pavementBand,
                 222, 0, 202);
    target->copy(m_strip, m_fenceBand, 0, VISIBLE_WIDTH + m_fenceBand, 94, 0,
                 0);
    target->copy(m_strip, m_roadBand, 95, VISIBLE_WIDTH + m_roadBand, 201, 0,
                 95);
  }
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
  m_buffer.drawBobs(m_bobs, m_images);
  m_buffer.swap();
  return wait(1, Step::DriveBottom);
}

CarStage::Flow CarStage::driveBottom() {
  m_buffer.clearBobs();
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
    m_machine.create(CAR_CHANNEL, actors::compiled::carDriveOff());
    m_machine.start(CAR_CHANNEL);
  }
  if ((m_distance == 0 && !m_machine.isRunning(CAR_CHANNEL)) ||
      global(amal::RG) < 0 || m_escape) {
    m_session.lastDrive = {m_ignition, m_roadBand, m_fenceBand, m_clock,
                           m_engineBeat};
    m_buffer.setUpdates(true);
    return hold(SCREEN_CLOSE_VBLS, Step::StripClosed);
  }
  sys();
  m_step = Step::DriveTop;
  return Flow::Continue;
}

void CarStage::runOver() {
  for (int channel = 1; channel <= PEDESTRIANS; ++channel) {
    const int bob = FIRST_PEDESTRIAN + channel - 1;
    const int y = m_bobs.y(bob);
    const int carY = m_bobs.y(CAR);
    if (y <= carY - HIT_REACH_ABOVE || y >= carY + HIT_REACH_BELOW ||
        m_hit[static_cast<std::size_t>(channel)] ||
        !m_bobs.collide(CAR, m_images, bob, bob) || !m_bobs.collided(bob)) {
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
  m_strip = core::IndexedSurface(0, 0);
  if (global(amal::RG) < 0 || m_escape) {
    gameOver();
    return Flow::Yield;
  }
  test();
  m_machine.destroyAll();
  m_bobs.offAll();
  return autoback([](core::IndexedSurface &surface) { surface.fill(0); },
                  Step::Cleared);
}

void CarStage::gameOver() {
  Stage::gameOver();
  m_step = Step::GameOver;
}

void CarStage::runBasic(const StreetInput &input) {
  Flow flow = Flow::Continue;
  while (flow == Flow::Continue && m_frame >= m_resumeFrame) {
    switch (m_step) {
    case Step::Password:
      clearScreen();
      flow = Flow::Yield;
      break;
    case Step::PasswordText:
      password();
      flow = Flow::Yield;
      break;
    case Step::HideForLoading:
      hideForLoading();
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
      m_road = core::IndexedSurface(ROAD_WIDTH, SCREEN_HEIGHT);
      flow = hold(SCREEN_OPEN_VBLS, Step::RoadOpened);
      break;
    case Step::RoadOpened:
      openRoad();
      flow = hold(SCREEN_OPEN_VBLS, Step::StripOpened);
      break;
    case Step::StripOpened:
      openStrip();
      flow = hold(SCREEN_CLOSE_VBLS, Step::RoadClosed);
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
    case Step::DriveScenery:
      flow = driveScenery();
      break;
    case Step::DriveBottom:
      flow = driveBottom();
      break;
    case Step::StripClosed:
      flow = leave();
      break;
    case Step::Cleared:
      m_session.fromBonusDrive = true;
      m_outcome = Outcome::Cleared;
      m_step = Step::Finished;
      flow = Flow::Yield;
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
