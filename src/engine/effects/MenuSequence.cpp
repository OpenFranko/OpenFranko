#include "MenuSequence.h"

#include "AmigaDisplay.h"

#include <utility>
#include <vector>

namespace openfranko::src::engine::effects {
namespace {

struct Icon {
  int bob;
  int16_t y;
  bool left;
  bool GameOptions::*option;
};

constexpr std::array<Icon, 6> ICONS = {{
    {4, 20, true, nullptr},
    {5, 90, true, &GameOptions::music},
    {6, 160, true, &GameOptions::bass},
    {7, 20, false, &GameOptions::mono},
    {8, 90, false, &GameOptions::ntsc},
    {9, 160, false, &GameOptions::tallScreen},
}};
constexpr int ROWS = 3;
constexpr int16_t LEFT_OUTSIDE = -64;
constexpr int16_t RIGHT_OUTSIDE = 384;

constexpr int HAND_BOB = 10;
constexpr std::array<int16_t, 2> HAND_X = {128, 240};
constexpr std::array<int16_t, ROWS> HAND_Y = {32, 102, 172};

struct Credit {
  int bob;
  int16_t y;
  int firstImage;
  int lastImage;
};

struct Layout {
  std::array<int, 6> iconImages;
  int startPressedImage;
  int handImage;
  std::array<Credit, 3> credits;
  int16_t creditX;
};

constexpr Layout VERSION10_LAYOUT = {
    {42, 44, 46, 48, 50, 52},
    43,
    54,
    {{{1, 350, 55, 67}, {2, 490, 56, 68}, {3, 630, 57, 69}}},
    180};

constexpr Layout VERSION12_LAYOUT = {
    {1, 3, 5, 7, 9, 11},
    2,
    13,
    {{{1, 350, 14, 23}, {2, 490, 15, 24}, {3, 630, 16, 25}}},
    184};

const Layout &layout(GameVersion version) {
  return version == GameVersion::V12 ? VERSION12_LAYOUT : VERSION10_LAYOUT;
}

constexpr int DIRECTIONS = 4;
constexpr int VERSION12_MUSIC_WAIT = 2;
constexpr int VERSION12_LEAVING_CLOSE_AT = 50;

constexpr int ICON_PAIR_EVERY = 10;
constexpr int CREDITS_AT = 50;
constexpr int HAND_MOVE_WAIT = 10;
constexpr int MACH_WAIT = 40;
constexpr int LEAVING_FADE_AT = 50;
constexpr int LEAVING_FADE_SPEED = 3;
constexpr int LEAVING_CLOSE_AT = 95;
constexpr int UNPACK_VBLS = 1;
constexpr int DOUBLE_BUFFER_VBLS = 3;
constexpr char FIRST_TYPED = ' ';

const std::vector<AmalMotion::Move> FLY_RIGHT = {{88, 16}, {8, 8}};
const std::vector<AmalMotion::Move> FLY_LEFT = {{-88, 16}, {-8, 8}};
const std::vector<AmalMotion::Move> WAGGLE = {
    {4, 2}, {-4, 2}, {0, 1}, {4, 2}, {-4, 2}, {0, 1},
    {4, 2}, {-4, 2}, {0, 1}, {4, 2}, {-4, 2}, {0, 1}};

int iconImage(const Icon &icon, const GameOptions &options,
              GameVersion version) {
  const std::size_t index = static_cast<std::size_t>(&icon - ICONS.data());
  const int image = layout(version).iconImages[index];
  if (icon.option == nullptr) {
    return image;
  }
  return image + (options.*icon.option ? 1 : 0);
}

bool pressed(const MenuSequence::Joystick &joystick, int direction) {
  switch (direction) {
  case 0:
    return joystick.left;
  case 1:
    return joystick.right;
  case 2:
    return joystick.up;
  default:
    return joystick.down;
  }
}

} // namespace

MenuSequence::MenuSequence(GameOptions &options, AmigaPalette palette,
                           InkeyBuffer &keyboard, GameVersion version)
    : m_options(options), m_version(version), m_palette(std::move(palette)),
      m_keyboard(keyboard) {
  if (m_version == GameVersion::V10) {
    m_keyboard.forbid();
  } else {
    m_phaseStart = VERSION12_MUSIC_WAIT;
  }
  for (const Icon &icon : ICONS) {
    bob(icon.bob) = {true, icon.left ? LEFT_OUTSIDE : RIGHT_OUTSIDE, icon.y,
                     iconImage(icon, m_options, m_version), false};
  }
  placeHand();
  m_shownBobs = m_bobs;
}

void MenuSequence::setMouseButton(bool down) { m_mouseButton = down; }

void MenuSequence::advance(const Joystick &joystick) {
  m_shownBobs = m_bobs;
  m_keysRead.clear();
  runScript(joystick);
  if (m_attractDue) {
    return;
  }
  runAmal();
  m_fader.tick(m_palette);
  ++m_frame;
  ++m_timer;
}

void MenuSequence::resumeAfterAttract() {
  m_attractDue = false;
  m_timer = 0;
  m_resume = Resume::Choosing;
  m_resumeFrame = m_frame + SCREEN_CLOSE_VBLS;
  m_busy = true;
}

const std::array<MenuSequence::Bob, MenuSequence::BOBS> &
MenuSequence::bobs() const {
  return m_bobs;
}

const std::array<MenuSequence::Bob, MenuSequence::BOBS> &
MenuSequence::shownBobs() const {
  return m_shownBobs;
}

const AmigaPalette &MenuSequence::palette() const { return m_palette; }

const std::string &MenuSequence::keysRead() const { return m_keysRead; }

bool MenuSequence::isAttractDue() const { return m_attractDue; }

bool MenuSequence::isScreenShown() const { return m_screenShown; }

bool MenuSequence::isFinished() const { return m_phase == Phase::Finished; }

void MenuSequence::runScript(const Joystick &joystick) {
  if (m_resume != Resume::Nothing) {
    if (m_frame < m_resumeFrame) {
      if (!m_busy) {
        m_keyboard.sleep();
      }
      return;
    }
    m_busy = false;
    const Resume resume = std::exchange(m_resume, Resume::Nothing);
    m_timer = 0;
    if (resume == Resume::Hand) {
      finishPass();
    }
    if (resume == Resume::Leaving) {
      m_phase = Phase::Leaving;
      m_phaseStart = m_frame;
    }
  }

  if (m_phase == Phase::Unpacking) {
    if (m_frame - m_phaseStart < UNPACK_VBLS + DOUBLE_BUFFER_VBLS) {
      return;
    }
    m_screenShown = true;
    m_phase = Phase::Opening;
    m_phaseStart = m_frame;
  }

  const int time = m_frame - m_phaseStart;
  switch (m_phase) {
  case Phase::Unpacking:
    break;
  case Phase::Opening:
    if (time % ICON_PAIR_EVERY == 0 && time / ICON_PAIR_EVERY < ROWS) {
      flyIcons(time / ICON_PAIR_EVERY, true);
    }
    if (time == CREDITS_AT) {
      startCredits();
      m_phase = Phase::Choosing;
      m_timer = 0;
      choose(joystick);
    } else {
      m_keyboard.sleep();
    }
    break;
  case Phase::Choosing:
    choose(joystick);
    break;
  case Phase::Leaving:
    if (time % ICON_PAIR_EVERY == 0 && time / ICON_PAIR_EVERY < ROWS) {
      flyIcons(time / ICON_PAIR_EVERY, false);
    }
    if (m_version == GameVersion::V10 && time == LEAVING_FADE_AT) {
      m_fader.start(m_palette, LEAVING_FADE_SPEED,
                    AmigaPalette(m_palette.size(), 0));
    }
    if (time == (m_version == GameVersion::V12 ? VERSION12_LEAVING_CLOSE_AT
                                               : LEAVING_CLOSE_AT)) {
      m_phase = Phase::Closing;
      m_phaseStart = m_frame;
    }
    break;
  case Phase::Closing:
    if (time == SCREEN_CLOSE_SHOWN_VBLS) {
      for (Bob &shown : m_bobs) {
        shown.shown = false;
      }
      m_screenShown = false;
    }
    if (time == SCREEN_CLOSE_VBLS) {
      m_phase = Phase::Finished;
    }
    break;
  case Phase::Finished:
    break;
  }
}

void MenuSequence::choose(const Joystick &joystick) {
  if (m_version == GameVersion::V12) {
    chooseInTurn(joystick);
    return;
  }
  if (m_timer > ATTRACT_AFTER) {
    m_attractDue = true;
    return;
  }
  if (joystick.fire) {
    activate();
  } else if (joystick.up || joystick.down || joystick.left || joystick.right) {
    moveHand(joystick);
  } else {
    readKeys();
  }
}

void MenuSequence::chooseInTurn(const Joystick &joystick) {
  if (m_direction == 0 && m_timer > ATTRACT_AFTER) {
    m_attractDue = true;
    return;
  }
  for (; m_direction < DIRECTIONS; ++m_direction) {
    if (!pressed(joystick, m_direction)) {
      continue;
    }
    if (m_direction < 2) {
      m_column = m_direction;
    } else {
      m_row = (m_row + (m_direction == 3 ? 1 : ROWS - 1)) % ROWS;
    }
    placeHand();
    ++m_direction;
    m_resume = Resume::Choosing;
    m_resumeFrame = m_frame + HAND_MOVE_WAIT;
    return;
  }
  m_direction = 0;
  if (joystick.fire) {
    activate();
  } else {
    readKeys();
  }
}

void MenuSequence::finishPass() {
  const std::optional<char> key = m_keyboard.inkey();
  if (key && *key >= FIRST_TYPED) {
    m_keysRead += *key;
    if (m_version == GameVersion::V10) {
      m_timer = 0;
    }
  }
  if (m_mouseButton) {
    m_keyboard.permit();
  }
}

void MenuSequence::readKeys() {
  do {
    finishPass();
  } while (!m_keyboard.isEmpty());
}

void MenuSequence::moveHand(const Joystick &joystick) {
  if (joystick.left) {
    m_column = 0;
  }
  if (joystick.right) {
    m_column = 1;
  }
  const int step = (joystick.down ? 1 : 0) - (joystick.up ? 1 : 0);
  m_row = (m_row + step + ROWS) % ROWS;
  placeHand();
  m_resume = Resume::Hand;
  m_resumeFrame = m_frame + HAND_MOVE_WAIT;
}

void MenuSequence::activate() {
  const Icon &icon = ICONS[m_column * ROWS + m_row];
  if (icon.option == nullptr) {
    bob(icon.bob).image = layout(m_version).startPressedImage;
    m_resume = Resume::Leaving;
  } else {
    m_options.*icon.option = !(m_options.*icon.option);
    bob(icon.bob).image = iconImage(icon, m_options, m_version);
    m_resume = Resume::Choosing;
  }
  m_resumeFrame = m_frame + MACH_WAIT;
  m_motions[HAND_BOB - 1] = AmalMotion(WAGGLE);
}

void MenuSequence::flyIcons(std::size_t row, bool in) {
  for (std::size_t i = row; i < ICONS.size(); i += ROWS) {
    const Icon &icon = ICONS[i];
    const bool towardsRight = icon.left == in;
    m_motions[icon.bob - 1] = AmalMotion(towardsRight ? FLY_RIGHT : FLY_LEFT);
  }
}

void MenuSequence::startCredits() {
  const Layout &shown = layout(m_version);
  for (std::size_t i = 0; i < shown.credits.size(); ++i) {
    const Credit &credit = shown.credits[i];
    m_credits[i].emplace(credit.y, credit.firstImage, credit.lastImage);
    bob(credit.bob) = {true, shown.creditX, credit.y, credit.firstImage, false};
  }
}

void MenuSequence::runAmal() {
  const Layout &shown = layout(m_version);
  for (std::size_t i = 0; i < shown.credits.size(); ++i) {
    if (m_credits[i]) {
      m_credits[i]->advance();
      Bob &credit = bob(shown.credits[i].bob);
      credit.y = m_credits[i]->y();
      credit.image = m_credits[i]->image();
    }
  }
  for (std::size_t i = 0; i < BOBS; ++i) {
    m_bobs[i].x = m_motions[i].advance(m_bobs[i].x);
  }
}

void MenuSequence::placeHand() {
  bob(HAND_BOB) = {true, HAND_X[m_column], HAND_Y[m_row],
                   layout(m_version).handImage, m_column == 0};
}

MenuSequence::Bob &MenuSequence::bob(int number) { return m_bobs[number - 1]; }

} // namespace openfranko::src::engine::effects
