#ifndef ENGINE_EFFECTS_INKEYBUFFER_H_
#define ENGINE_EFFECTS_INKEYBUFFER_H_

#include <cstddef>
#include <deque>
#include <optional>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

class InkeyBuffer {
public:
  static constexpr std::size_t CAPACITY = 31;

  void press(char key);
  void permit();
  void forbid();
  void sleep();
  std::optional<char> inkey();
  bool isEmpty() const;

private:
  void deliver();

  std::deque<char> m_pressed;
  std::deque<char> m_buffer;
  bool m_permitted = false;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_INKEYBUFFER_H_
