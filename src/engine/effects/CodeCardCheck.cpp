#include "CodeCardCheck.h"

#include <stdexcept>
#include <utility>

namespace openfranko::src::engine::effects {

CodeCardCheck::CodeCardCheck(std::vector<uint8_t> cards,
                             std::array<Cell, CARDS> cells)
    : m_cards(std::move(cards)), m_cells(cells) {
  if (m_cards.size() != CARDS * CARD_BYTES) {
    throw std::invalid_argument("The code cards need 200 bytes");
  }
  for (const Cell &cell : m_cells) {
    if (cell.x < 0 || cell.x >= CARD_SIZE || cell.y < 0 ||
        cell.y >= CARD_SIZE) {
      throw std::invalid_argument("A code card cell is off the card");
    }
  }
}

void CodeCardCheck::answer(char letter) {
  if (isFinished() || letter < FIRST_ANSWER || letter > LAST_ANSWER) {
    return;
  }

  const Cell &asked = m_cells[m_question];
  const uint8_t expected =
      m_cards[m_question * CARD_BYTES + asked.x + CARD_SIZE * asked.y];
  if (letter - FIRST_ANSWER != expected) {
    m_passed = false;
  }
  ++m_question;
}

CodeCardCheck::Cell CodeCardCheck::cell() const {
  return m_cells[isFinished() ? CARDS - 1 : m_question];
}

bool CodeCardCheck::isFinished() const { return m_question == CARDS; }

bool CodeCardCheck::isPassed() const { return isFinished() && m_passed; }

} // namespace openfranko::src::engine::effects
