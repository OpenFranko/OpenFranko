#ifndef SYSTEMS_CONTROLLERSYSTEM_H_
#define SYSTEMS_CONTROLLERSYSTEM_H_

#include <array>
#include <cstdint>
#include <optional>

namespace openfranko {
namespace src {
namespace systems {

enum class FunctionKey { F1, F2, F3, F4, Escape };

class ControllerSystem {
public:
  void update();
  void clearFireLatch();
  bool isFireLatched() const;
  std::optional<char> typedLetter() const;
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
  void clearStates();
  void updateTypedLetter(const uint8_t *keys);
  void updateFunctionKey(const uint8_t *keys);

  bool fireLatched = false;
  std::array<bool, 26> lettersDown{};
  std::optional<char> letter;
  std::array<bool, 5> functionKeysDown{};
  std::optional<FunctionKey> pressedFunctionKey;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_CONTROLLERSYSTEM_H_