#include "EntityComponentSystem.h"

namespace openfranko::src::entityComponentSystem {

void Entity::addGroup(Group group) {
  groupBitset[group] = true;
  manager.addToGroup(this, group);
}

} // namespace openfranko::src::entityComponentSystem