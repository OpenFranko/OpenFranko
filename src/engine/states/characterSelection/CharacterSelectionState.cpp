#include "CharacterSelectionState.h"

#include <array>
#include <cstddef>
#include <string>

namespace openfranko::src::engine::states::characterSelection {
namespace {

constexpr auto PICTURE = "characterPicture";
constexpr auto PICTURE_PATH = "assets/03B9.bmp";

constexpr int SCREEN_ID = 0;
constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 256;
constexpr std::size_t SCREEN_COLORS = 32;

struct Sprite {
  int image;
  const char *path;
};

constexpr std::array<Sprite, 3> SPRITES = {{
    {1, "assets/0035/0035_000.bmp"},
    {2, "assets/0035/0035_001.bmp"},
    {3, "assets/0035/0035_002.bmp"},
}};

struct Voice {
  int sample;
  const char *name;
  const char *path;
};

constexpr std::array<Voice, 2> VOICES = {{
    {1, "characterFranko", "assets/0035/0035_sam1_6063Hz.wav"},
    {2, "characterAlex", "assets/0035/0035_sam2_6563Hz.wav"},
}};

constexpr int HIDDEN_IMAGE = 0;

constexpr int RO = 14;
constexpr int SECOND_STAGE = 2;
constexpr int THIRD_STAGE = 3;

std::string spriteName(int image) {
  return "characterSprite" + std::to_string(image);
}

effects::CharacterSelection::Joystick
joystickFrom(const systems::ControllerSystem::ControllerStates &states) {
  return {states.left, states.right, states.button};
}

} // namespace

CharacterSelectionState::CharacterSelectionState(
    systems::VideoSystem &videoSystem, systems::AudioSystem &audioSystem,
    systems::ControllerSystem &controllerSystem, effects::GameOptions &options,
    street::GameSession &session)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_controllerSystem(controllerSystem), m_session(session),
      m_selection(options, session.nameScreenOpen ? 1 : 0) {
  m_videoSystem.setNtsc(false);
  m_videoSystem.createScreen(SCREEN_ID, SCREEN_WIDTH, SCREEN_HEIGHT);
  m_videoSystem.switchScreen(SCREEN_ID);
  m_videoSystem.loadIndexedImage(PICTURE, PICTURE_PATH);
  auto palette = m_videoSystem.getImagePalette(PICTURE);
  palette.resize(SCREEN_COLORS);
  for (const Sprite &sprite : SPRITES) {
    m_videoSystem.loadMaskedImage(spriteName(sprite.image), sprite.path);
    m_videoSystem.setImagePalette(spriteName(sprite.image), palette);
  }
  for (const Voice &voice : VOICES) {
    m_audioSystem.loadSFX(voice.name, voice.path);
  }
}

CharacterSelectionState::~CharacterSelectionState() {
  for (const Voice &voice : VOICES) {
    m_audioSystem.clearSFX(voice.name);
  }
  m_videoSystem.clearImage(PICTURE);
  for (const Sprite &sprite : SPRITES) {
    m_videoSystem.clearImage(spriteName(sprite.image));
  }
  m_videoSystem.fillScreen(0, 0, 0);
}

std::optional<EngineStateEnum> CharacterSelectionState::update() {
  if (m_selection.isFinished()) {
    return firstStreet();
  }

  m_selection.advance(joystickFrom(m_controllerSystem.states));

  if (const auto sample = m_selection.sample()) {
    for (const Voice &voice : VOICES) {
      if (voice.sample == *sample) {
        m_audioSystem.playSFXSilencingMusic(voice.name);
      }
    }
  }
  if (m_selection.stopsMusic()) {
    m_audioSystem.stopMusic();
  }
  if (const auto volume = m_selection.musicVolume()) {
    m_audioSystem.setMusicVolume(*volume);
  }

  if (m_selection.isFinished()) {
    m_session.nameScreenOpen = false;
    m_videoSystem.fillScreen(0, 0, 0);
    return firstStreet();
  }
  draw();
  return std::nullopt;
}

EngineStateEnum CharacterSelectionState::firstStreet() const {
  switch (m_session.registers[RO] + 1) {
  case SECOND_STAGE:
    return EngineStateEnum::Level2;
  case THIRD_STAGE:
    return EngineStateEnum::StageProtectionCheck;
  default:
    return EngineStateEnum::Level1;
  }
}

void CharacterSelectionState::draw() {
  if (!m_selection.isScreenShown()) {
    m_videoSystem.fillScreen(0, 0, 0);
    return;
  }
  m_videoSystem.drawImage(PICTURE, 0, 0);
  for (const effects::CharacterSelection::Bob *bob :
       {&m_selection.face(), &m_selection.hand()}) {
    if (bob->shown && bob->image != HIDDEN_IMAGE) {
      m_videoSystem.drawImage(spriteName(bob->image), bob->x, bob->y,
                              bob->flipped ? SDL_FLIP_HORIZONTAL
                                           : SDL_FLIP_NONE);
    }
  }
}

} // namespace openfranko::src::engine::states::characterSelection
