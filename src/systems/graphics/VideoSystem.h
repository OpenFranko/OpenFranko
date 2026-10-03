#ifndef SYSTEMS_GRAPHICS_VIDEOSYSTEM_H_
#define SYSTEMS_GRAPHICS_VIDEOSYSTEM_H_

#include "graphics/Display.h"
#include "graphics/Monitor.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {

class VideoSystem : public Monitor {
public:
  VideoSystem();
  ~VideoSystem() override;

  VideoSystem(const VideoSystem &) = delete;
  VideoSystem &operator=(const VideoSystem &) = delete;

  void show(const Display &display) override;
  void clear();
  void sync();
  void setNtsc(bool enabled) override;
  bool isNtsc() const override;
  bool readsBuffersLive() const override;
  bool showsSprites() const override;
  int refreshRate() const;

private:
  struct Window;

  void present();
  void settle(int slot);
  void waitVbl();

  std::unique_ptr<Window> m_window;
  Display m_shown;
  int m_shownSlot = -1;
  uint32_t m_shownRevision = 0;
  std::vector<uint32_t> m_frame;
  bool m_frameChanged = false;
  bool m_ntsc = false;
};

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_VIDEOSYSTEM_H_
