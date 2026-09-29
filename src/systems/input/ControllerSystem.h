#ifndef SYSTEMS_INPUT_CONTROLLERSYSTEM_H_
#define SYSTEMS_INPUT_CONTROLLERSYSTEM_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>

namespace openfranko {
namespace src {
namespace systems {
namespace input {

inline constexpr int16_t JOY_UP = 1;
inline constexpr int16_t JOY_DOWN = 2;
inline constexpr int16_t JOY_LEFT = 4;
inline constexpr int16_t JOY_RIGHT = 8;
inline constexpr int16_t JOY_FIRE = 16;

enum class FunctionKey { F1, F2, F3, F4, Escape, Other };

enum class KeyMode { Game, FrontEnd, NameEntry };

enum class Key {
  Other,
  Up,
  Down,
  Left,
  Right,
  Space,
  W,
  A,
  S,
  D,
  Delete,
  F1,
  F2,
  F3,
  F4,
  F9,
  Escape
};

struct KeyEvent {
  Key key = Key::Other;
  int code = 0;
  char character = 0;
  bool pressed = false;
  bool repeat = false;
};

class ControllerSystem {
public:
  void update();
  void receiveKey(const KeyEvent &event);
  void receiveMouseButton(bool pressed);
  void setKeyMode(KeyMode mode);
  void clearFireLatch();
  bool isFireLatched() const;
  bool isMouseButtonDown() const;
  bool isDeleteHeld() const;
  bool isKeyHeld(Key key) const;
  const std::string &typedKeys() const;
  std::optional<FunctionKey> functionKey() const;
  int16_t joystick() const;

  struct ControllerStates {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool button = false;
  };

  ControllerStates states;

private:
  static constexpr std::size_t KEYS = static_cast<std::size_t>(Key::Escape) + 1;

  std::optional<char> typedCharacter(const KeyEvent &event) const;
  bool isJoystickKey(Key key) const;
  bool isHeld(Key key) const;

  KeyMode m_keyMode = KeyMode::FrontEnd;
  bool m_fireLatched = false;
  bool m_mouseButtonHeld = false;
  bool m_mouseButtonDown = false;
  bool m_deleteHeld = false;
  std::array<bool, KEYS> m_heldKeys{};
  std::string m_receivedKeys;
  std::string m_typed;
  std::set<int> m_typingKeys;
  std::optional<FunctionKey> m_receivedKeyEvent;
  std::optional<FunctionKey> m_keyEvent;
};

} // namespace input
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_INPUT_CONTROLLERSYSTEM_H_
