#ifndef ENTITYCOMPONENTSYSTEM_POSITIONCOMPONENT_POSITIONCOMPONENT_H_
#define ENTITYCOMPONENTSYSTEM_POSITIONCOMPONENT_POSITIONCOMPONENT_H_

#include "../EntityComponentSystem.h"

namespace openfranko {
namespace src {
namespace entityComponentSystem {
namespace positionComponent {

class PositionComponent : public entityComponentSystem::Component {
private:
  int xpos;
  int ypos;

public:
  PositionComponent() {
    xpos = 0;
    ypos = 0;
  }

  PositionComponent(int x, int y) {
    xpos = x;
    ypos = y;
  }

  void update() override {
    xpos++;
    ypos++;
  }

  int x() { return xpos; }
  void x(int x) { xpos = x; }
  int y() { return ypos; }
  void y(int y) { ypos = y; }

  void setPos(int x, int y) {
    xpos = x;
    ypos = y;
  }
};

} // namespace positionComponent
} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_POSITIONCOMPONENT_POSITIONCOMPONENT_H_