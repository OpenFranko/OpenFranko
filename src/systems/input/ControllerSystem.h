#ifndef SYSTEMS_CONTROLLERSYSTEM_H_
#define SYSTEMS_CONTROLLERSYSTEM_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>

namespace openfranko {
namespace src {
namespace systems {

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

  KeyMode keyMode = KeyMode::FrontEnd;
  bool fireLatched = false;
  bool mouseButtonHeld = false;
  bool mouseButtonDown = false;
  bool deleteHeld = false;
  std::array<bool, KEYS> heldKeys{};
  std::string receivedKeys;
  std::string typed;
  std::set<int> typingKeys;
  std::optional<FunctionKey> receivedKeyEvent;
  std::optional<FunctionKey> keyEvent;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_CONTROLLERSYSTEM_H_
