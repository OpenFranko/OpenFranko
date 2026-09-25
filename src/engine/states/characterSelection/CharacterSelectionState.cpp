#include "CharacterSelectionState.h"

#include <array>
#include <cstddef>

namespace openfranko::src::engine::states::characterSelection {
namespace {

constexpr auto PICTURE_PATH = "assets/03B9.bmp";

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 256;
constexpr int DISPLAY_LINE = 40;
constexpr std::size_t SCREEN_COLORS = 32;

constexpr int FIRST_SPRITE_IMAGE = 1;
constexpr std::array<const char *, 3> SPRITE_PATHS = {{
    "assets/0035/0035_000.bmp",
    "assets/0035/0035_001.bmp",
    "assets/0035/0035_002.bmp",
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

effects::AmigaPalette screenPalette(const systems::IndexedBitmap &picture) {
  effects::AmigaPalette palette = picture.palette;
  palette.resize(SCREEN_COLORS);
  return palette;
}

std::vector<systems::IndexedBitmap> loadSprites() {
  std::vector<systems::IndexedBitmap> sprites;
  for (const char *path : SPRITE_PATHS) {
    sprites.push_back(systems::loadIndexedBitmap(path));
  }
  return sprites;
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
      m_selection(options, session.nameScreenOpen ? 1 : 0),
      m_rows(
          effects::visibleRows(effects::pictureLine(DISPLAY_LINE, options.ntsc),
                               SCREEN_HEIGHT, options.ntsc)),
      m_picture(systems::loadIndexedBitmap(PICTURE_PATH)),
      m_screenPalette(screenPalette(m_picture)), m_sprites(loadSprites()),
      m_screen(SCREEN_WIDTH, m_rows.count) {
  m_videoSystem.setNtsc(options.ntsc);
  for (const Voice &voice : VOICES) {
    m_audioSystem.loadSFX(voice.name, voice.path);
  }
}

CharacterSelectionState::~CharacterSelectionState() {
  for (const Voice &voice : VOICES) {
    m_audioSystem.clearSFX(voice.name);
  }
}

std::optional<EngineStateEnum> CharacterSelectionState::update() {
  if (m_selection.isFinished()) {
    return firstStreet();
  }

  const bool shown = m_selection.isScreenShown();
  m_selection.advance(joystickFrom(m_controllerSystem.states));
  if (!shown && m_selection.isScreenShown()) {
    m_session.border = m_screenPalette[0];
  }

  if (const auto sample = m_selection.sample()) {
    for (const Voice &voice : VOICES) {
      if (voice.sample == *sample) {
        m_audioSystem.playSample(voice.name, systems::AudioSystem::ALL_VOICES);
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
    m_screen.fill(m_session.border);
  } else {
    m_screen.setPalette(m_picture.palette);
    m_screen.draw(m_picture, 0, -m_rows.first);
    for (const effects::CharacterSelection::Bob *bob :
         {&m_selection.face(), &m_selection.hand()}) {
      const int sprite = bob->image - FIRST_SPRITE_IMAGE;
      if (bob->shown && bob->image != HIDDEN_IMAGE && sprite >= 0 &&
          sprite < static_cast<int>(m_sprites.size())) {
        m_screen.drawMasked(m_sprites[static_cast<std::size_t>(sprite)], bob->x,
                            bob->y - m_rows.first, bob->flipped);
      }
    }
  }
  m_videoSystem.show(m_screen.output());
}

} // namespace openfranko::src::engine::states::characterSelection
