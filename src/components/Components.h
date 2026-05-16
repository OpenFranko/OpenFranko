#ifndef COMPONENTS_COMPONENETS_H_
#define COMPONENTS_COMPONENETS_H_

#include "../entityComponentSystem/EntityComponentSystem.h"

namespace openfranko {
namespace src {
namespace components {

class PositionComponent : public entityComponentSystem::Component {
private:
  int xpos;
  int ypos;

public:
  int x() { return xpos; }
  int y() { return ypos; }

  void init() override {
    xpos = 0;
    ypos = 0;
  }

  void update() override {
    xpos++;
    ypos++;
  }

  void setPos(int x, int y) {
    xpos = x;
    ypos = y;
  }
};

} // namespace components
} // namespace src
} // namespace openfranko

#endif // COMPONENTS_COMPONENETS_H_