#ifndef ENGINE_EFFECTS_CODECARDCHECK_H_
#define ENGINE_EFFECTS_CODECARDCHECK_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

class CodeCardCheck {
public:
  struct Cell {
    int x;
    int y;
  };

  static constexpr int CARD_SIZE = 10;
  static constexpr std::size_t CARDS = 2;
  static constexpr std::size_t CARD_BYTES = CARD_SIZE * CARD_SIZE;
  static constexpr char FIRST_ANSWER = 'A';
  static constexpr char LAST_ANSWER = 'K';

  CodeCardCheck(std::vector<uint8_t> cards, std::array<Cell, CARDS> cells);

  void answer(char letter);

  Cell cell() const;
  bool isFinished() const;
  bool isPassed() const;

private:
  std::vector<uint8_t> m_cards;
  std::array<Cell, CARDS> m_cells;
  std::size_t m_question = 0;
  bool m_passed = true;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_CODECARDCHECK_H_
