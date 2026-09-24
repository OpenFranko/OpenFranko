#include "MenuSequence.h"

#include <utility>
#include <vector>

namespace openfranko::src::engine::effects {
namespace {

struct Icon {
  int bob;
  int16_t y;
  bool left;
  int image;
  bool GameOptions::*option;
};

constexpr std::array<Icon, 6> ICONS = {{
    {4, 20, true, 42, nullptr},
    {5, 90, true, 44, &GameOptions::music},
    {6, 160, true, 46, &GameOptions::bass},
    {7, 20, false, 48, &GameOptions::mono},
    {8, 90, false, 50, &GameOptions::ntsc},
    {9, 160, false, 52, &GameOptions::tallScreen},
}};
constexpr int ROWS = 3;
constexpr int START_PRESSED_IMAGE = 43;
constexpr int16_t LEFT_OUTSIDE = -64;
constexpr int16_t RIGHT_OUTSIDE = 384;

constexpr int HAND_BOB = 10;
constexpr int HAND_IMAGE = 54;
constexpr std::array<int16_t, 2> HAND_X = {128, 240};
constexpr std::array<int16_t, ROWS> HAND_Y = {32, 102, 172};

struct Credit {
  int bob;
  int16_t y;
  int firstImage;
  int lastImage;
};

constexpr std::array<Credit, 3> CREDITS = {{
    {1, 350, 55, 67},
    {2, 490, 56, 68},
    {3, 630, 57, 69},
}};
constexpr int16_t CREDIT_X = 180;

constexpr int ICON_PAIR_EVERY = 10;
constexpr int CREDITS_AT = 50;
constexpr int HAND_MOVE_WAIT = 10;
constexpr int MACH_WAIT = 40;
constexpr int LEAVING_FADE_AT = 50;
constexpr int LEAVING_FADE_SPEED = 3;
constexpr int LEAVING_CLOSE_AT = 95;

const std::vector<AmalMotion::Move> FLY_RIGHT = {{88, 16}, {8, 8}};
const std::vector<AmalMotion::Move> FLY_LEFT = {{-88, 16}, {-8, 8}};
const std::vector<AmalMotion::Move> WAGGLE = {
    {4, 2}, {-4, 2}, {0, 1}, {4, 2}, {-4, 2}, {0, 1},
    {4, 2}, {-4, 2}, {0, 1}, {4, 2}, {-4, 2}, {0, 1}};

int iconImage(const Icon &icon, const GameOptions &options) {
  if (icon.option == nullptr) {
    return icon.image;
  }
  return icon.image + (options.*icon.option ? 1 : 0);
}

} // namespace

MenuSequence::MenuSequence(GameOptions &options, AmigaPalette palette)
    : m_options(options), m_palette(std::move(palette)) {
  for (const Icon &icon : ICONS) {
    bob(icon.bob) = {true, icon.left ? LEFT_OUTSIDE : RIGHT_OUTSIDE, icon.y,
                     iconImage(icon, m_options), false};
  }
  placeHand();
  m_shownBobs = m_bobs;
}

void MenuSequence::press(char key) { m_keyboard.press(key); }

void MenuSequence::setMouseButton(bool down) { m_mouseButton = down; }

void MenuSequence::sleep() { m_keyboard.sleep(); }

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

bool MenuSequence::isFinished() const { return m_phase == Phase::Finished; }

void MenuSequence::runScript(const Joystick &joystick) {
  if (m_resume != Resume::Nothing) {
    if (m_frame < m_resumeFrame) {
      m_keyboard.sleep();
      return;
    }
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

  const int time = m_frame - m_phaseStart;
  switch (m_phase) {
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
    if (time == LEAVING_FADE_AT) {
      m_fader.start(m_palette, LEAVING_FADE_SPEED,
                    AmigaPalette(m_palette.size(), 0));
    }
    if (time == LEAVING_CLOSE_AT) {
      m_phase = Phase::Finished;
      for (Bob &shown : m_bobs) {
        shown.shown = false;
      }
    }
    break;
  case Phase::Finished:
    break;
  }
}

void MenuSequence::choose(const Joystick &joystick) {
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

void MenuSequence::finishPass() {
  if (const std::optional<char> key = m_keyboard.inkey()) {
    m_keysRead += *key;
    m_timer = 0;
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
    bob(icon.bob).image = START_PRESSED_IMAGE;
    m_resume = Resume::Leaving;
  } else {
    m_options.*icon.option = !(m_options.*icon.option);
    bob(icon.bob).image = iconImage(icon, m_options);
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
  for (std::size_t i = 0; i < CREDITS.size(); ++i) {
    const Credit &credit = CREDITS[i];
    m_credits[i].emplace(credit.y, credit.firstImage, credit.lastImage);
    bob(credit.bob) = {true, CREDIT_X, credit.y, credit.firstImage, false};
  }
}

void MenuSequence::runAmal() {
  for (std::size_t i = 0; i < CREDITS.size(); ++i) {
    if (m_credits[i]) {
      m_credits[i]->advance();
      Bob &credit = bob(CREDITS[i].bob);
      credit.y = m_credits[i]->y();
      credit.image = m_credits[i]->image();
    }
  }
  for (std::size_t i = 0; i < BOBS; ++i) {
    m_bobs[i].x = m_motions[i].advance(m_bobs[i].x);
  }
}

void MenuSequence::placeHand() {
  bob(HAND_BOB) = {true, HAND_X[m_column], HAND_Y[m_row], HAND_IMAGE,
                   m_column == 0};
}

MenuSequence::Bob &MenuSequence::bob(int number) { return m_bobs[number - 1]; }

} // namespace openfranko::src::engine::effects
