#include "CharacterSelectionState.h"

#include "../../assets/Assets.h"

#include <array>
#include <cstddef>

namespace openfranko::src::engine::states::characterSelection {
namespace {

constexpr int PICTURE = 0x3B9;
constexpr int SPRITE_SET = 0x35;
constexpr int SPRITES = 3;

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 256;
constexpr int DISPLAY_LINE = 40;
constexpr std::size_t SCREEN_COLORS = 32;

constexpr int FIRST_SPRITE_IMAGE = 1;

struct Voice {
  int sample;
  const char *name;
};

constexpr std::array<Voice, 2> VOICES = {{
    {1, "characterFranko"},
    {2, "characterAlex"},
}};

constexpr int HIDDEN_IMAGE = 0;

constexpr int RO = 14;
constexpr int SECOND_STAGE = 2;
constexpr int THIRD_STAGE = 3;

effects::color::AmigaPalette
screenPalette(const systems::graphics::IndexedBitmap &picture) {
  effects::color::AmigaPalette palette = picture.palette;
  palette.resize(SCREEN_COLORS);
  return palette;
}

std::vector<systems::graphics::IndexedBitmap> loadSprites(GameVersion version) {
  const std::string name = assets::resourceName(SPRITE_SET, version);
  std::vector<systems::graphics::IndexedBitmap> sprites;
  for (int index = 0; index < SPRITES; ++index) {
    sprites.push_back(
        systems::graphics::loadIndexedBitmap(assets::imagePath(name, index)));
  }
  return sprites;
}

effects::sequences::CharacterSelectionSequence::Joystick
joystickFrom(const systems::input::ControllerSystem::ControllerStates &states) {
  return {states.left, states.right, states.button};
}

} // namespace

CharacterSelectionState::CharacterSelectionState(
    systems::graphics::VideoSystem &videoSystem,
    systems::audio::AudioSystem &audioSystem,
    systems::input::ControllerSystem &controllerSystem, GameOptions &options,
    street::session::GameSession &session)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_controllerSystem(controllerSystem), m_session(session),
      m_selection(options, session.nameScreenOpen ? 1 : 0, session.version),
      m_rows(visibleRows(pictureLine(DISPLAY_LINE, options.ntsc), SCREEN_HEIGHT,
                         options.ntsc)),
      m_picture(systems::graphics::loadIndexedBitmap(
          assets::picturePath(assets::resourceName(PICTURE, session.version)))),
      m_screenPalette(screenPalette(m_picture)),
      m_sprites(loadSprites(session.version)),
      m_screen(SCREEN_WIDTH, m_rows.count) {
  m_videoSystem.setNtsc(options.ntsc);
  const std::string voices = assets::resourceName(SPRITE_SET, session.version);
  for (const Voice &voice : VOICES) {
    m_audioSystem.loadSample(voice.name,
                             assets::samplePath(voices, voice.sample));
  }
}

CharacterSelectionState::~CharacterSelectionState() {
  for (const Voice &voice : VOICES) {
    m_audioSystem.clearSample(voice.name);
  }
}

std::optional<EngineStateId> CharacterSelectionState::update() {
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
        m_audioSystem.playSample(voice.name,
                                 systems::audio::AudioSystem::ALL_VOICES);
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

EngineStateId CharacterSelectionState::firstStreet() const {
  switch (m_session.registers[RO] + 1) {
  case SECOND_STAGE:
    return EngineStateId::Level2;
  case THIRD_STAGE:
    return EngineStateId::StageProtectionCheck;
  default:
    return EngineStateId::Level1;
  }
}

void CharacterSelectionState::draw() {
  if (!m_selection.isScreenShown()) {
    m_screen.fill(m_session.border);
  } else {
    m_screen.setPalette(m_picture.palette);
    m_screen.draw(m_picture, 0, -m_rows.first);
    for (const effects::animation::Bob *bob :
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
