#ifndef SYSTEMS_PLATFORM_H_
#define SYSTEMS_PLATFORM_H_

#include "input/ControllerSystem.h"

namespace openfranko {
namespace src {
namespace systems {
namespace input {

class Platform {
public:
  Platform();
  ~Platform();

  Platform(const Platform &) = delete;
  Platform &operator=(const Platform &) = delete;

  bool pollEvents(ControllerSystem &controller);
};

} // namespace input
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_PLATFORM_H_
