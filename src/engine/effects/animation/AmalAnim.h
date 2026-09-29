#ifndef ENGINE_EFFECTS_ANIMATION_AMALANIM_H_
#define ENGINE_EFFECTS_ANIMATION_AMALANIM_H_

#include <cstddef>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {
namespace animation {

class AmalAnim {
public:
  struct Frame {
    int image = 0;
    int frames = 0;
  };

  AmalAnim() = default;
  AmalAnim(std::vector<Frame> frames, int loops);

  int advance(int image);
  bool isFinished() const;

private:
  std::vector<Frame> m_frames;
  int m_loops = 0;
  std::size_t m_index = 0;
  int m_wait = 0;
  bool m_started = false;
  bool m_finished = true;
};

} // namespace animation
} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_ANIMATION_AMALANIM_H_
