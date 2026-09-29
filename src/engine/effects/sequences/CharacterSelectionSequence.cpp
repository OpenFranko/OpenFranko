#include "CharacterSelectionSequence.h"

#include "../../AmigaDisplay.h"

#include <vector>

namespace openfranko::src::engine::effects::sequences {
namespace {

struct Pose {
  int16_t handX = 0;
  bool handFlipped = false;
  int16_t faceX = 0;
  int faceImage = 0;
  int sample = 0;
};

constexpr Pose FRANKO = {64, false, 148, 2, 1};
constexpr Pose ALEX = {280, true, 189, 3, 2};

constexpr int16_t HAND_Y = 150;
constexpr int HAND_IMAGE = 1;
constexpr int16_t FACE_Y = 117;
constexpr int HIDDEN_IMAGE = 0;
constexpr int FACE_FRAMES = 10;
constexpr int FACE_LOOPS = 2;

constexpr int MACH_WAIT = 40;
constexpr int SHOW_WAIT = 50;
constexpr int VERSION12_SHOW_WAIT = 150;
constexpr int LOUDEST = 63;

const std::vector<animation::AmalMotion::Move> WAGGLE = {
    {4, 2}, {-4, 2}, {0, 1}, {4, 2}, {-4, 2}, {0, 1},
    {4, 2}, {-4, 2}, {0, 1}, {4, 2}, {-4, 2}, {0, 1}};

const Pose &poseOf(Character character) {
  return character == Character::Franko ? FRANKO : ALEX;
}

} // namespace

CharacterSelectionSequence::CharacterSelectionSequence(GameOptions &options,
                                                       int otherScreens,
                                                       GameVersion version)
    : m_options(options), m_version(version), m_otherScreens(otherScreens) {
  choose(Character::Franko);
}

void CharacterSelectionSequence::advance(const Joystick &joystick) {
  m_sample.reset();
  m_musicVolume.reset();
  m_stopsMusic = false;
  if (m_finished) {
    return;
  }
  if (m_frame < UNPACK_VBLS) {
    ++m_frame;
    return;
  }
  if (!m_closedAt) {
    m_screenShown = true;
  }

  if (!m_confirmedAt) {
    if (joystick.left) {
      choose(Character::Franko);
    }
    if (joystick.right) {
      choose(Character::Alex);
    }
    if (joystick.fire) {
      m_confirmedAt = m_frame;
      m_handMotion = animation::AmalMotion(WAGGLE);
    }
  } else {
    runScript(m_frame - *m_confirmedAt);
  }

  if (m_hand.shown) {
    m_hand.x = m_handMotion.advance(m_hand.x);
  }
  if (m_face.shown) {
    m_face.image = m_faceAnim.advance(m_face.image);
  }
  ++m_frame;
}

const animation::Bob &CharacterSelectionSequence::hand() const {
  return m_hand;
}

const animation::Bob &CharacterSelectionSequence::face() const {
  return m_face;
}

std::optional<int> CharacterSelectionSequence::sample() const {
  return m_sample;
}

std::optional<int> CharacterSelectionSequence::musicVolume() const {
  return m_musicVolume;
}

bool CharacterSelectionSequence::stopsMusic() const { return m_stopsMusic; }

bool CharacterSelectionSequence::isScreenShown() const { return m_screenShown; }

bool CharacterSelectionSequence::isFinished() const { return m_finished; }

void CharacterSelectionSequence::choose(Character character) {
  m_options.character = character;
  const Pose &pose = poseOf(character);
  m_hand = {true, pose.handX, HAND_Y, HAND_IMAGE, pose.handFlipped};
}

void CharacterSelectionSequence::runScript(int time) {
  if (m_closedAt) {
    if (time == *m_closedAt + SCREEN_CLOSE_SHOWN_VBLS) {
      m_screenShown = false;
    }
    if (time == *m_closedAt + SCREEN_CLOSE_VBLS * (1 + m_otherScreens)) {
      m_finished = true;
    }
    return;
  }
  const bool version12 = m_version == GameVersion::V12;
  const int bobsOffAt =
      MACH_WAIT + (version12 ? VERSION12_SHOW_WAIT : SHOW_WAIT);
  const int musicStopAt = bobsOffAt + LOUDEST + 1;
  const Pose &pose = poseOf(m_options.character);
  if (time == MACH_WAIT) {
    m_face = {true, pose.faceX, FACE_Y, HIDDEN_IMAGE, false};
    m_faceAnim = animation::AmalAnim(
        {{HIDDEN_IMAGE, FACE_FRAMES}, {pose.faceImage, FACE_FRAMES}},
        FACE_LOOPS);
    m_sample = pose.sample;
  }
  if (time == bobsOffAt) {
    m_hand.shown = false;
    m_face.shown = false;
  }
  if (!m_options.music && !version12) {
    if (time == bobsOffAt) {
      m_stopsMusic = true;
      close(time);
    }
    return;
  }
  if (time >= bobsOffAt && time < musicStopAt) {
    m_musicVolume = LOUDEST - (time - bobsOffAt);
  }
  if (time == musicStopAt) {
    m_stopsMusic = true;
    if (!version12) {
      m_musicVolume = LOUDEST;
    }
    close(time);
  }
}

void CharacterSelectionSequence::close(int time) { m_closedAt = time; }

} // namespace openfranko::src::engine::effects::sequences
