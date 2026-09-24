#include "CharacterSelection.h"

#include <vector>

namespace openfranko::src::engine::effects {
namespace {

struct Pose {
  int16_t handX;
  bool handFlipped;
  int16_t faceX;
  int faceImage;
  int sample;
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
constexpr int BOBS_OFF_AT = MACH_WAIT + SHOW_WAIT;
constexpr int LOUDEST = 63;
constexpr int MUSIC_STOP_AT = BOBS_OFF_AT + LOUDEST + 1;

const std::vector<AmalMotion::Move> WAGGLE = {
    {4, 2}, {-4, 2}, {0, 1}, {4, 2}, {-4, 2}, {0, 1},
    {4, 2}, {-4, 2}, {0, 1}, {4, 2}, {-4, 2}, {0, 1}};

const Pose &poseOf(Character character) {
  return character == Character::Franko ? FRANKO : ALEX;
}

} // namespace

CharacterSelection::CharacterSelection(GameOptions &options)
    : m_options(options) {
  choose(Character::Franko);
}

void CharacterSelection::advance(const Joystick &joystick) {
  m_sample.reset();
  m_musicVolume.reset();
  m_stopsMusic = false;
  if (m_finished) {
    return;
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
      m_handMotion = AmalMotion(WAGGLE);
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

const CharacterSelection::Bob &CharacterSelection::hand() const {
  return m_hand;
}

const CharacterSelection::Bob &CharacterSelection::face() const {
  return m_face;
}

std::optional<int> CharacterSelection::sample() const { return m_sample; }

std::optional<int> CharacterSelection::musicVolume() const {
  return m_musicVolume;
}

bool CharacterSelection::stopsMusic() const { return m_stopsMusic; }

bool CharacterSelection::isFinished() const { return m_finished; }

void CharacterSelection::choose(Character character) {
  m_options.character = character;
  const Pose &pose = poseOf(character);
  m_hand = {true, pose.handX, HAND_Y, HAND_IMAGE, pose.handFlipped};
}

void CharacterSelection::runScript(int time) {
  const Pose &pose = poseOf(m_options.character);
  if (time == MACH_WAIT) {
    m_face = {true, pose.faceX, FACE_Y, HIDDEN_IMAGE, false};
    m_faceAnim =
        AmalAnim({{HIDDEN_IMAGE, FACE_FRAMES}, {pose.faceImage, FACE_FRAMES}},
                 FACE_LOOPS);
    m_sample = pose.sample;
  }
  if (time == BOBS_OFF_AT) {
    m_hand.shown = false;
    m_face.shown = false;
  }
  if (!m_options.music) {
    if (time == BOBS_OFF_AT) {
      m_stopsMusic = true;
      m_finished = true;
    }
    return;
  }
  if (time >= BOBS_OFF_AT && time < MUSIC_STOP_AT) {
    m_musicVolume = LOUDEST - (time - BOBS_OFF_AT);
  }
  if (time == MUSIC_STOP_AT) {
    m_stopsMusic = true;
    m_musicVolume = LOUDEST;
    m_finished = true;
  }
}

} // namespace openfranko::src::engine::effects
