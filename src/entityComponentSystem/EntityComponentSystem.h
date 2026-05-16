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

using ComponentId = size_t;

inline ComponentId getComponentTypeID() {
  static ComponentId lastId = 0;
  return lastId++;
}

template <typename T> inline ComponentId getComponentTypeID() noexcept {
  static ComponentId typeId = getComponentTypeID();
  return typeId;
}

constexpr size_t maxComponents = 32;

using ComponentBitset = std::bitset<maxComponents>;
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
  bool active = true;
  std::vector<std::unique_ptr<Component>> components;

  ComponentArray componentArray;
  ComponentBitset componentBitset;

public:
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
    entities.erase(std::remove_if(std::begin(entities), std::end(entities),
                                  [](const std::unique_ptr<Entity> &mEntity) {
                                    return !mEntity->isActive();
                                  }),
                   std::end(entities));
  }

  Entity &addEntity() {
    Entity *e = new Entity();
    std::unique_ptr<Entity> uPtr{e};
    entities.emplace_back(std::move(uPtr));
    return *e;
  }
};

} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_ENTITYCOMPONENTSYSTEM_H_