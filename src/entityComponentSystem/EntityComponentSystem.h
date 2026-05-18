#ifndef ENTITYCOMPONENTSYSTEM_ENTITYCOMPONENTSYSTEM_H_
#define ENTITYCOMPONENTSYSTEM_ENTITYCOMPONENTSYSTEM_H_

#include <algorithm>
#include <array>
#include <bitset>
#include <iostream>
#include <memory>
#include <vector>

namespace openfranko {
namespace src {
namespace entityComponentSystem {

class Component;
class Entity;
class Manager;

using ComponentId = size_t;
using Group = size_t;
inline ComponentId getNewComponentTypeID() {
  static ComponentId lastId = 0u;
  return lastId++;
}

template <typename T> inline ComponentId getComponentTypeID() noexcept {
  static ComponentId typeId = getNewComponentTypeID();
  return typeId;
}

constexpr size_t maxComponents = 32;
constexpr size_t maxGroups = 32;

using ComponentBitset = std::bitset<maxComponents>;
using GroupBitset = std::bitset<maxGroups>;

using ComponentArray = std::array<Component *, maxComponents>;

class Component {
public:
  Entity *entity;

  virtual void init() {}
  virtual void update() {}
  virtual void draw() {}

  virtual ~Component() {}
};

class Entity {
private:
  Manager &manager;
  bool active = true;
  std::vector<std::unique_ptr<Component>> components;

  ComponentArray componentArray;
  ComponentBitset componentBitset;
  GroupBitset groupBitset;

public:
  Entity(Manager &man) : manager{man} {}

  void update() {
    for (auto &component : components) {
      component->update();
    }
  }

  void draw() {
    for (auto &component : components) {
      component->draw();
    }
  }

  bool isActive() const { return active; }

  void destroy() { active = false; }

  bool hasGroup(Group group) { return groupBitset[group]; }

  void addGroup(Group group);
  void delGroup(Group group) { groupBitset[group] = false; }

  template <typename T> bool hasComponent() const {
    return componentBitset[getComponentTypeID<T>()];
  }

  template <typename T, typename... TArgs> T &addComponent(TArgs &&...mArgs) {
    T *c(new T(std::forward<TArgs>(mArgs)...));
    c->entity = this;
    std::unique_ptr<Component> uPtr{c};
    components.emplace_back(std::move(uPtr));

    componentArray[getComponentTypeID<T>()] = c;
    componentBitset[getComponentTypeID<T>()] = true;

    c->init();
    return *c;
  }

  template <typename T> T &getComponent() const {
    auto ptr(componentArray[getComponentTypeID<T>()]);
    return *static_cast<T *>(ptr);
  }
};

class Manager {
private:
  std::vector<std::unique_ptr<Entity>> entities;
  std::array<std::vector<Entity *>, maxGroups> groupedEntities;

public:
  void update() {
    for (auto &entity : entities) {
      entity->update();
    }
  }

  void draw() {
    for (auto &entity : entities) {
      entity->draw();
    }
  }

  void refresh() {

    for (auto i(0u); i < maxGroups; i++) {
      auto &v(groupedEntities[i]);
      v.erase(std::remove_if(std::begin(v), std::end(v),
                             [i](Entity *mEntity) {
                               return !mEntity->isActive() ||
                                      !mEntity->hasGroup(i);
                             }),
              std::end(v));
    }

    entities.erase(std::remove_if(std::begin(entities), std::end(entities),
                                  [](const std::unique_ptr<Entity> &mEntity) {
                                    return !mEntity->isActive();
                                  }),
                   std::end(entities));
  }

  void addToGroup(Entity *mEntity, Group mGroup) {
    groupedEntities[mGroup].emplace_back(mEntity);
  }

  std::vector<Entity *> &getGroup(Group mGroup) {
    return groupedEntities[mGroup];
  }

  Entity &addEntity() {
    Entity *e = new Entity(*this);
    std::unique_ptr<Entity> uPtr{e};
    entities.emplace_back(std::move(uPtr));
    return *e;
  }
};

} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_ENTITYCOMPONENTSYSTEM_H_