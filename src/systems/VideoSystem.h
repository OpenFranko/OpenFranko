#ifndef SYSTEMS_VIDEOSYSTEM_H_
#define SYSTEMS_VIDEOSYSTEM_H_

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

  void show(const uint32_t *argb, int width, int height, int displayHeight = 0);
  void sync();
  void setNtsc(bool enabled);
  bool isNtsc() const;
  int refreshRate() const;

private:
  struct Window;

  std::unique_ptr<Window> window;
  std::vector<uint32_t> frame;
  int frameWidth = 0;
  int frameHeight = 0;
  int frameDisplayHeight = 0;
  bool frameChanged = false;
  bool ntsc = false;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_VIDEOSYSTEM_H_
