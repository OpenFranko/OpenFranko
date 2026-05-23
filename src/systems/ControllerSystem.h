#ifndef SYSTEMS_CONTROLLERSYSTEM_H_
#define SYSTEMS_CONTROLLERSYSTEM_H_

namespace openfranko {
namespace src {
namespace systems {

class ControllerSystem {
public:
  void update();

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
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_CONTROLLERSYSTEM_H_