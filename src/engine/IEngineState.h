#ifndef ENGINE_IENGINESTATE_H_
#define ENGINE_IENGINESTATE_H_

#include "EngineStateEnum.h"
#include <optional>

class IEngineState {
public:
  virtual ~IEngineState() = default;
  virtual std::optional<EngineStateEnum> update() = 0;
};

#endif // ENGINE_IENGINESTATE_H_