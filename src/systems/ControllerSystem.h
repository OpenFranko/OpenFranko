#ifndef SYSTEMS_CONTROLLERSYSTEM_H_
#define SYSTEMS_CONTROLLERSYSTEM_H_

#include <SDL2/SDL.h>
#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace openfranko {
namespace src {
namespace systems {

enum class FunctionKey { F1, F2, F3, F4, Escape };

enum class KeyMode { Game, FrontEnd, NameEntry };

class ControllerSystem {
public:
  void update();
  void receiveKey(const SDL_KeyboardEvent &key);
  void setKeyMode(KeyMode mode);
  void clearFireLatch();
  bool isFireLatched() const;
  bool isMouseButtonDown() const;
  bool isDeleteHeld() const;
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
  std::optional<char> typedCharacter(SDL_Keycode keycode) const;
  void updateFunctionKey(const uint8_t *keys);

  KeyMode keyMode = KeyMode::FrontEnd;
  bool fireLatched = false;
  bool mouseButtonDown = false;
  bool deleteHeld = false;
  std::string receivedKeys;
  std::string typed;
  std::array<bool, SDL_NUM_SCANCODES> typingKeys{};
  std::array<bool, 5> functionKeysDown{};
  std::optional<FunctionKey> pressedFunctionKey;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_CONTROLLERSYSTEM_H_
