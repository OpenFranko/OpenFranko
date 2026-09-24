#include "MenuState.h"

#include "../../effects/AmigaDisplay.h"
#include "../../street/CheatCodes.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>

namespace openfranko::src::engine::states::menu {
namespace {

constexpr auto BACKDROP = "menuBackdrop";
constexpr auto BACKDROP_PATH = "assets/03B8.bmp";
constexpr auto TITLE = "menuTitle";
constexpr auto TITLE_PATH = "assets/03BA.bmp";
constexpr auto HISCORES = "menuHiscores";
constexpr auto HISCORES_PATH = "assets/03B9.bmp";

constexpr int MENU_SCREEN_ID = 0;
constexpr int MENU_SCREEN_WIDTH = 368;
constexpr int MENU_SCREEN_HEIGHT = 290;
constexpr int MENU_DISPLAY_Y = 32;
constexpr std::size_t MENU_COLORS = 16;
constexpr int ATTRACT_SCREEN_ID = 1;
constexpr int ATTRACT_SCREEN_WIDTH = 320;
constexpr int ATTRACT_SCREEN_HEIGHT = 256;
constexpr int ATTRACT_DISPLAY_Y = 40;
constexpr int ATTRACT_NTSC_RAISE = 27;
constexpr std::size_t ATTRACT_COLORS = 32;

constexpr int FIRST_MENU_IMAGE = 42;
constexpr int LAST_MENU_IMAGE = 69;
constexpr int LETTER_IMAGES = 41;
constexpr int LETTER_A_IMAGE = 14;
constexpr int DIGIT_IMAGE_OFFSET = 44;

constexpr int MUSIC_ON_VOLUME = 63;

constexpr int NAME_X = 56;
constexpr int SCORE_RIGHT = 272;
constexpr int CHARACTER_PITCH = 10;
constexpr int SCORE_CHARACTERS = 4;
constexpr int FIRST_ROW_Y = 32;
constexpr int ROW_PITCH = 20;

constexpr int RO = 14;

struct VisibleRows {
  int first;
  int count;
};

VisibleRows visibleRows(int displayY, int height, bool ntscDisplay) {
  const int first = std::max(0, effects::FIRST_VISIBLE_LINE - displayY);
  const int last =
      std::min(height - 1, effects::lastVisibleLine(ntscDisplay) - displayY);
  return {first, std::max(0, last - first + 1)};
}

void createMenuScreen(systems::VideoSystem &videoSystem, bool ntscDisplay) {
  videoSystem.createScreen(
      MENU_SCREEN_ID, MENU_SCREEN_WIDTH,
      visibleRows(MENU_DISPLAY_Y, MENU_SCREEN_HEIGHT, ntscDisplay).count);
}

std::string spritePath(const char *resource, int index) {
  char path[64];
  std::snprintf(path, sizeof(path), "assets/%s/%s_%03d.bmp", resource, resource,
                index);
  return path;
}

std::string menuBobName(int image) { return "menuBob" + std::to_string(image); }

std::string letterName(int image) {
  return "hiscoreLetter" + std::to_string(image);
}

effects::AmigaPalette loadPicture(systems::VideoSystem &videoSystem,
                                  const char *name, const char *path,
                                  std::size_t colors) {
  videoSystem.loadIndexedImage(name, path);
  auto palette = videoSystem.getImagePalette(name);
  palette.resize(colors);
  return palette;
}

effects::AmigaPalette openMenuScreen(systems::VideoSystem &videoSystem) {
  videoSystem.setNtsc(false);
  createMenuScreen(videoSystem, false);
  videoSystem.switchScreen(MENU_SCREEN_ID);
  return loadPicture(videoSystem, BACKDROP, BACKDROP_PATH, MENU_COLORS);
}

effects::MenuSequence::Joystick
joystickFrom(const systems::ControllerSystem::ControllerStates &states) {
  return {states.up, states.down, states.left, states.right, states.button};
}

bool isTouched(const effects::MenuSequence::Joystick &joystick) {
  return joystick.up || joystick.down || joystick.left || joystick.right ||
         joystick.fire;
}

} // namespace

MenuState::MenuState(systems::VideoSystem &videoSystem,
                     systems::AudioSystem &audioSystem,
                     systems::ControllerSystem &controllerSystem,
                     effects::GameOptions &options,
                     street::GameSession &session)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_controllerSystem(controllerSystem), m_options(options),
      m_session(session), m_menu(options, openMenuScreen(videoSystem)) {
  for (int image = FIRST_MENU_IMAGE; image <= LAST_MENU_IMAGE; ++image) {
    m_videoSystem.loadMaskedImage(menuBobName(image),
                                  spritePath("0034", image - FIRST_MENU_IMAGE));
  }
  for (int image = 1; image <= LETTER_IMAGES; ++image) {
    m_videoSystem.loadMaskedImage(letterName(image),
                                  spritePath("0035", image - 1));
  }
  m_titlePalette =
      loadPicture(m_videoSystem, TITLE, TITLE_PATH, ATTRACT_COLORS);
  m_hiscorePalette =
      loadPicture(m_videoSystem, HISCORES, HISCORES_PATH, ATTRACT_COLORS);
}

MenuState::~MenuState() {
  m_videoSystem.clearImage(BACKDROP);
  m_videoSystem.clearImage(TITLE);
  m_videoSystem.clearImage(HISCORES);
  for (int image = FIRST_MENU_IMAGE; image <= LAST_MENU_IMAGE; ++image) {
    m_videoSystem.clearImage(menuBobName(image));
  }
  for (int image = 1; image <= LETTER_IMAGES; ++image) {
    m_videoSystem.clearImage(letterName(image));
  }
  m_videoSystem.fillScreen(0, 0, 0);
}

std::optional<EngineStateEnum> MenuState::update() {
  const effects::MenuSequence::Joystick joystick =
      joystickFrom(m_controllerSystem.states);
  for (const char key : m_controllerSystem.typedText()) {
    m_menu.press(key);
  }

  if (m_attract) {
    advanceAttract(joystick);
    if (!m_attract->isFinished()) {
      drawAttract();
      return std::nullopt;
    }
    m_attract.reset();
    m_menu.resumeAfterAttract();
  }

  const bool music = m_options.music;
  const bool bass = m_options.bass;
  const bool ntsc = m_options.ntsc;
  m_menu.setMouseButton(m_controllerSystem.isMouseButtonDown());
  m_menu.advance(joystick);
  for (const char key : m_menu.keysRead()) {
    street::typeCheatKey(m_session.textBuffer, key);
  }
  if (m_options.music != music) {
    m_audioSystem.setMusicVolume(m_options.music ? MUSIC_ON_VOLUME : 0);
  }
  if (m_options.bass != bass) {
    m_audioSystem.setLowPassFilter(m_options.bass);
  }
  if (m_options.ntsc != ntsc) {
    switchStandard();
  }
  if (m_menu.isFinished()) {
    m_session.registers[RO] = 0;
    street::applyCheatCodes(m_session);
    return EngineStateEnum::CharacterSelection;
  }

  if (m_menu.isAttractDue()) {
    startAttract();
    advanceAttract(joystick);
    drawAttract();
    return std::nullopt;
  }

  drawMenu();
  return std::nullopt;
}

void MenuState::advanceAttract(
    const effects::MenuSequence::Joystick &joystick) {
  m_attract->advance(isTouched(joystick));
  if (m_attract->isWaiting()) {
    m_menu.sleep();
  }
}

void MenuState::switchStandard() {
  m_videoSystem.setNtsc(m_options.ntsc);
  m_audioSystem.setMusicTempoScale(
      effects::menuTuneScale(effects::menuTempo(m_options.ntsc)));
  createMenuScreen(m_videoSystem, m_options.ntsc);
}

void MenuState::startAttract() {
  const VisibleRows rows =
      visibleRows(ATTRACT_DISPLAY_Y - (m_options.ntsc ? ATTRACT_NTSC_RAISE : 0),
                  ATTRACT_SCREEN_HEIGHT, m_videoSystem.isNtsc());
  m_attractTop = rows.first;
  m_videoSystem.createScreen(ATTRACT_SCREEN_ID, ATTRACT_SCREEN_WIDTH,
                             rows.count);
  const effects::AttractSequence::Kind kind = m_nextAttract;
  const bool title = kind == effects::AttractSequence::Kind::Title;
  m_nextAttract = title ? effects::AttractSequence::Kind::Hiscores
                        : effects::AttractSequence::Kind::Title;
  m_attract.emplace(kind, title ? m_titlePalette : m_hiscorePalette);
  m_attractPaletteShown.clear();
}

void MenuState::drawMenu() {
  m_videoSystem.switchScreen(MENU_SCREEN_ID);
  if (m_menu.isFinished()) {
    m_videoSystem.fillScreen(0, 0, 0);
    return;
  }

  if (m_menu.palette() != m_menuPaletteShown) {
    m_menuPaletteShown = m_menu.palette();
    m_videoSystem.setImagePalette(BACKDROP, m_menuPaletteShown);
    for (int image = FIRST_MENU_IMAGE; image <= LAST_MENU_IMAGE; ++image) {
      m_videoSystem.setImagePalette(menuBobName(image), m_menuPaletteShown);
    }
  }

  m_videoSystem.drawImage(BACKDROP, 0, 0);
  for (const effects::MenuSequence::Bob &bob : m_menu.shownBobs()) {
    if (bob.shown) {
      m_videoSystem.drawImage(menuBobName(bob.image), bob.x, bob.y,
                              bob.flipped ? SDL_FLIP_HORIZONTAL
                                          : SDL_FLIP_NONE);
    }
  }
}

void MenuState::drawAttract() {
  if (!m_attract->isShowing()) {
    drawMenu();
    return;
  }

  m_videoSystem.switchScreen(ATTRACT_SCREEN_ID);
  if (m_attract->kind() == effects::AttractSequence::Kind::Title) {
    m_videoSystem.drawImage(TITLE, 0, -m_attractTop);
    return;
  }

  if (m_attract->palette() != m_attractPaletteShown) {
    m_attractPaletteShown = m_attract->palette();
    m_videoSystem.setImagePalette(HISCORES, m_attractPaletteShown);
    for (int image = 1; image <= LETTER_IMAGES; ++image) {
      m_videoSystem.setImagePalette(letterName(image), m_attractPaletteShown);
    }
  }

  m_videoSystem.drawImage(HISCORES, 0, -m_attractTop);
  for (int drawn = 0; drawn < m_attract->rowsShown(); ++drawn) {
    drawHiscoreRow(effects::AttractSequence::HISCORE_ROWS - 1 - drawn);
  }
}

void MenuState::drawHiscoreRow(int row) {
  const street::HighScoreTable &table = m_session.highScores;
  const int y = FIRST_ROW_Y + row * ROW_PITCH - m_attractTop;

  for (int column = 0; column < street::HighScoreTable::NAME_LENGTH; ++column) {
    const int letter = table.letter(row, column);
    if (letter < street::HighScoreTable::LETTERS) {
      m_videoSystem.drawImage(letterName(letter + LETTER_A_IMAGE),
                              NAME_X + column * CHARACTER_PITCH, y);
    }
  }

  const std::string score = " " + std::to_string(table.score(row)) + "   ";
  const int scoreX =
      SCORE_RIGHT - CHARACTER_PITCH * static_cast<int>(score.size());
  for (int i = 1; i <= SCORE_CHARACTERS; ++i) {
    const char character = score[i - 1];
    if (character > ' ') {
      m_videoSystem.drawImage(letterName(character - DIGIT_IMAGE_OFFSET),
                              scoreX + i * CHARACTER_PITCH, y);
    }
  }
}

} // namespace openfranko::src::engine::states::menu
