#include "MenuState.h"

#include "../../assets/Assets.h"
#include "../../effects/AmigaDisplay.h"
#include "../../street/CheatCodes.h"

#include <cstddef>
#include <string>

namespace openfranko::src::engine::states::menu {
namespace {

constexpr int BACKDROP = 0x3B8;
constexpr int TITLE = 0x3BA;
constexpr int HISCORES = 0x3B9;
constexpr int MENU_BOBS = 0x34;
constexpr int LETTERS = 0x35;
constexpr int MENU_TUNE = 0x261;

constexpr int MENU_SCREEN_WIDTH = 368;
constexpr int MENU_SCREEN_HEIGHT = 290;
constexpr int MENU_DISPLAY_Y = 32;
constexpr std::size_t MENU_COLORS = 16;
constexpr int ATTRACT_SCREEN_WIDTH = 320;
constexpr int ATTRACT_SCREEN_HEIGHT = 256;
constexpr int ATTRACT_DISPLAY_Y = 40;
constexpr std::size_t ATTRACT_COLORS = 32;

constexpr int FIRST_MENU_IMAGE = 42;
constexpr int LAST_MENU_IMAGE = 69;
constexpr int VERSION12_FIRST_MENU_IMAGE = 1;
constexpr int VERSION12_LAST_MENU_IMAGE = 25;
constexpr int FIRST_LETTER_IMAGE = 1;
constexpr int LETTER_IMAGES = 41;
constexpr int LETTER_A_IMAGE = 14;
constexpr int DIGIT_IMAGE_OFFSET = 44;

constexpr int MUSIC_ON_VOLUME = 63;
constexpr int VERSION12_MUSIC_WAIT = 2;

constexpr int NAME_X = 56;
constexpr int SCORE_RIGHT = 272;
constexpr int CHARACTER_PITCH = 10;
constexpr int SCORE_CHARACTERS = 4;
constexpr int FIRST_ROW_Y = 32;
constexpr int ROW_PITCH = 20;

constexpr int RO = 14;

systems::Canvas menuScreen(bool ntscDisplay) {
  return systems::Canvas(
      MENU_SCREEN_WIDTH,
      effects::visibleRows(MENU_DISPLAY_Y, MENU_SCREEN_HEIGHT, ntscDisplay)
          .count);
}

systems::IndexedBitmap loadPicture(int resource, GameVersion version) {
  return systems::loadIndexedBitmap(
      assets::picturePath(assets::resourceName(resource, version)));
}

std::vector<systems::IndexedBitmap> loadSprites(int resource, int count,
                                                GameVersion version) {
  const std::string name = assets::resourceName(resource, version);
  std::vector<systems::IndexedBitmap> sprites;
  for (int index = 0; index < count; ++index) {
    sprites.push_back(
        systems::loadIndexedBitmap(assets::imagePath(name, index)));
  }
  return sprites;
}

int firstMenuImage(GameVersion version) {
  return version == GameVersion::V12 ? VERSION12_FIRST_MENU_IMAGE
                                     : FIRST_MENU_IMAGE;
}

int menuImages(GameVersion version) {
  return version == GameVersion::V12
             ? VERSION12_LAST_MENU_IMAGE - VERSION12_FIRST_MENU_IMAGE + 1
             : LAST_MENU_IMAGE - FIRST_MENU_IMAGE + 1;
}

const systems::IndexedBitmap *
findImage(const std::vector<systems::IndexedBitmap> &images, int firstImage,
          int image) {
  const int index = image - firstImage;
  if (index < 0 || index >= static_cast<int>(images.size())) {
    return nullptr;
  }
  return &images[static_cast<std::size_t>(index)];
}

effects::AmigaPalette resized(effects::AmigaPalette palette,
                              std::size_t colors) {
  palette.resize(colors);
  return palette;
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
      m_session(session), m_backdrop(loadPicture(BACKDROP, session.version)),
      m_title(loadPicture(TITLE, session.version)),
      m_hiscores(loadPicture(HISCORES, session.version)),
      m_menuBobs(
          loadSprites(MENU_BOBS, menuImages(session.version), session.version)),
      m_letters(loadSprites(LETTERS, LETTER_IMAGES, session.version)),
      m_menuScreen(menuScreen(options.ntsc)),
      m_menu(options, resized(m_backdrop.palette, MENU_COLORS),
             session.keyboard, session.version),
      m_titlePalette(resized(m_title.palette, ATTRACT_COLORS)),
      m_hiscorePalette(resized(m_hiscores.palette, ATTRACT_COLORS)) {
  m_videoSystem.setNtsc(options.ntsc);
  m_session.nameScreenOpen = false;
  if (session.version == GameVersion::V12) {
    m_audioSystem.loadMusic(
        assets::musicPath(assets::resourceName(MENU_TUNE, session.version)));
    m_audioSystem.playMusic();
    m_musicWait = VERSION12_MUSIC_WAIT;
  }
}

std::optional<EngineStateEnum> MenuState::update() {
  const effects::MenuSequence::Joystick joystick =
      joystickFrom(m_controllerSystem.states);
  if (m_musicWait > 0 && --m_musicWait == 0) {
    m_audioSystem.setMusicTempo(effects::CONVERTED_MENU_TEMPO);
    m_audioSystem.setMusicVolume(m_options.music ? MUSIC_ON_VOLUME : 0);
  }

  if (m_attract && m_attractClosing == 0) {
    advanceAttract(joystick);
    if (!m_attract->isFinished()) {
      drawAttract();
      return std::nullopt;
    }
    m_menu.resumeAfterAttract();
    m_attractClosing = effects::SCREEN_CLOSE_SHOWN_VBLS;
  }

  const bool music = m_options.music;
  const bool bass = m_options.bass;
  const bool ntsc = m_options.ntsc;
  m_menu.setMouseButton(m_controllerSystem.isMouseButtonDown());
  const bool shown = m_menu.isScreenShown();
  m_menu.advance(joystick);
  if (!shown && m_menu.isScreenShown()) {
    m_session.border = m_menu.palette()[0];
  }
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

  if (m_attractClosing > 0) {
    drawAttractPicture();
    if (--m_attractClosing == 0) {
      m_attract.reset();
    }
    return std::nullopt;
  }

  drawMenu();
  return std::nullopt;
}

void MenuState::advanceAttract(
    const effects::MenuSequence::Joystick &joystick) {
  m_attract->advance(isTouched(joystick));
  if (m_attract->isWaiting()) {
    m_session.keyboard.sleep();
  }
}

void MenuState::switchStandard() {
  m_videoSystem.setNtsc(m_options.ntsc);
  if (m_session.version == GameVersion::V12) {
    m_audioSystem.setMusicTempo(effects::menuTempo(m_options.ntsc));
  } else {
    m_audioSystem.setMusicTempoScale(
        effects::menuTuneScale(effects::menuTempo(m_options.ntsc)));
  }
  m_menuScreen = menuScreen(m_options.ntsc);
}

void MenuState::startAttract() {
  const effects::VisibleRows rows = effects::visibleRows(
      effects::pictureLine(ATTRACT_DISPLAY_Y, m_options.ntsc),
      ATTRACT_SCREEN_HEIGHT, m_videoSystem.isNtsc());
  m_attractTop = rows.first;
  m_attractScreen = systems::Canvas(ATTRACT_SCREEN_WIDTH, rows.count);
  const effects::AttractSequence::Kind kind = m_nextAttract;
  const bool title = kind == effects::AttractSequence::Kind::Title;
  m_nextAttract = title ? effects::AttractSequence::Kind::Hiscores
                        : effects::AttractSequence::Kind::Title;
  m_attract.emplace(kind, title ? m_titlePalette : m_hiscorePalette);
}

void MenuState::drawMenu() {
  if (m_menu.isFinished() || !m_menu.isScreenShown()) {
    m_menuScreen.fill(m_session.border);
    show(m_menuScreen);
    return;
  }

  m_menuScreen.setPalette(m_menu.palette());
  m_menuScreen.draw(m_backdrop, 0, 0);
  for (const effects::MenuSequence::Bob &bob : m_menu.shownBobs()) {
    const systems::IndexedBitmap *image =
        findImage(m_menuBobs, firstMenuImage(m_session.version), bob.image);
    if (bob.shown && image) {
      m_menuScreen.drawMasked(*image, bob.x, bob.y, bob.flipped);
    }
  }
  show(m_menuScreen);
}

void MenuState::drawAttract() {
  if (!m_attract->isShowing()) {
    drawMenu();
    return;
  }
  drawAttractPicture();
}

void MenuState::drawAttractPicture() {
  if (m_attract->kind() == effects::AttractSequence::Kind::Title) {
    m_attractScreen.setPalette(m_title.palette);
    m_attractScreen.draw(m_title, 0, -m_attractTop);
    show(m_attractScreen);
    return;
  }

  m_attractScreen.setPalette(m_attract->palette());
  m_attractScreen.draw(m_hiscores, 0, -m_attractTop);
  for (int drawn = 0; drawn < m_attract->rowsShown(); ++drawn) {
    drawHiscoreRow(effects::AttractSequence::HISCORE_ROWS - 1 - drawn);
  }
  show(m_attractScreen);
}

void MenuState::drawHiscoreRow(int row) {
  const street::HighScoreTable &table = m_session.highScores;
  const int y = FIRST_ROW_Y + row * ROW_PITCH - m_attractTop;

  for (int column = 0; column < street::HighScoreTable::NAME_LENGTH; ++column) {
    const int letter = table.letter(row, column);
    const systems::IndexedBitmap *image =
        findImage(m_letters, FIRST_LETTER_IMAGE, letter + LETTER_A_IMAGE);
    if (letter < street::HighScoreTable::LETTERS && image) {
      m_attractScreen.drawMasked(*image, NAME_X + column * CHARACTER_PITCH, y);
    }
  }

  const std::string score = " " + std::to_string(table.score(row)) + "   ";
  const int scoreX =
      SCORE_RIGHT - CHARACTER_PITCH * static_cast<int>(score.size());
  for (int i = 1; i <= SCORE_CHARACTERS; ++i) {
    const char character = score[i - 1];
    const systems::IndexedBitmap *image = findImage(
        m_letters, FIRST_LETTER_IMAGE, character - DIGIT_IMAGE_OFFSET);
    if (character > ' ' && image) {
      m_attractScreen.drawMasked(*image, scoreX + i * CHARACTER_PITCH, y);
    }
  }
}

void MenuState::show(const systems::Canvas &screen) {
  m_videoSystem.show(screen.output());
}

} // namespace openfranko::src::engine::states::menu
