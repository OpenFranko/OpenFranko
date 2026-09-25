#ifndef SYSTEMS_VIDEOSYSTEM_H_
#define SYSTEMS_VIDEOSYSTEM_H_

#include "Display.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {

class VideoSystem {
public:
  VideoSystem();
  ~VideoSystem();

  VideoSystem(const VideoSystem &) = delete;
  VideoSystem &operator=(const VideoSystem &) = delete;

  void show(const Display &display);
  void clear();
  void sync();
  void setNtsc(bool enabled);
  bool isNtsc() const;
  int refreshRate() const;

private:
  struct Window;

  std::unique_ptr<Window> window;
  Display shown;
  std::vector<uint32_t> frame;
  bool frameChanged = false;
  bool ntsc = false;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_VIDEOSYSTEM_H_
