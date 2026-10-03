#ifndef ENGINE_MERSENNETWISTER_H_
#define ENGINE_MERSENNETWISTER_H_

#include <array>
#include <cstddef>
#include <cstdint>

namespace openfranko {
namespace src {
namespace engine {

class MersenneTwister {
public:
  using result_type = uint32_t;

  static constexpr std::size_t STATE_SIZE = 624;

  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return UINT32_MAX; }

  explicit MersenneTwister(result_type seed);

  result_type operator()();
  result_type upTo(result_type limit);

private:
  std::array<uint32_t, STATE_SIZE> m_state{};
  std::size_t m_index = 0;
};

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_MERSENNETWISTER_H_
