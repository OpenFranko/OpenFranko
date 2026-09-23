#ifndef ENGINE_EFFECTS_AMALMOTION_H_
#define ENGINE_EFFECTS_AMALMOTION_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

class AmalMotion {
public:
  struct Move {
    int16_t distance;
    int16_t frames;
  };

  AmalMotion() = default;
  explicit AmalMotion(std::vector<Move> moves);

  int16_t advance(int16_t position);
  bool isFinished() const;

private:
  void startNextMove();

  std::vector<Move> m_moves;
  std::size_t m_next = 0;
  int32_t m_step = 0;
  uint16_t m_fraction = 0;
  int m_framesLeft = 0;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_AMALMOTION_H_
