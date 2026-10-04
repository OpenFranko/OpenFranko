#ifndef SYSTEMS_GRAPHICS_MONITOR_H_
#define SYSTEMS_GRAPHICS_MONITOR_H_

#include "graphics/Display.h"

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {

class Monitor {
public:
  virtual ~Monitor() = default;

  virtual void show(const Display &display) = 0;
  virtual void setNtsc(bool enabled) = 0;
  virtual bool isNtsc() const = 0;
  virtual bool readsBuffersLive() const { return false; }
  virtual bool showsSprites() const { return false; }
  virtual bool diffsFrames() const { return false; }
};

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_MONITOR_H_
