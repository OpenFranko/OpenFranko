#include "CodeCardCheck.h"

#include <stdexcept>
#include <utility>

namespace openfranko::src::engine::effects {

CodeCardCheck::CodeCardCheck(std::vector<uint8_t> cards,
                             std::array<Cell, CARDS> cells)
    : CodeCardCheck(std::move(cards), {{0, cells[0]}, {1, cells[1]}}, false) {}

CodeCardCheck CodeCardCheck::stageCheck(std::vector<uint8_t> cards,
                                        std::array<Cell, STAGE_TRIES> tries) {
  std::vector<Question> questions;
  for (const Cell &cell : tries) {
    questions.push_back({0, cell});
  }
  return CodeCardCheck(std::move(cards), std::move(questions), true);
}

CodeCardCheck::CodeCardCheck(std::vector<uint8_t> cards,
                             std::vector<Question> questions,
                             bool firstRightPasses)
    : m_cards(std::move(cards)), m_questions(std::move(questions)),
      m_firstRightPasses(firstRightPasses) {
  if (m_cards.size() != CARDS * CARD_BYTES) {
    throw std::invalid_argument("The code cards need 200 bytes");
  }
  for (const Question &question : m_questions) {
    const Cell &cell = question.cell;
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

  const Question &asked = m_questions[m_question];
  const uint8_t expected =
      m_cards[asked.card * CARD_BYTES +
              static_cast<std::size_t>(asked.cell.x +
                                       CARD_SIZE * asked.cell.y)];
  if (letter - FIRST_ANSWER == expected) {
    m_anyRight = true;
  } else {
    m_anyWrong = true;
  }
  ++m_question;
}

CodeCardCheck::Cell CodeCardCheck::cell() const {
  return m_questions[isFinished() ? m_question - 1 : m_question].cell;
}

bool CodeCardCheck::isFinished() const {
  return m_question == m_questions.size() || (m_firstRightPasses && m_anyRight);
}

bool CodeCardCheck::isPassed() const {
  return isFinished() && (m_firstRightPasses ? m_anyRight : !m_anyWrong);
}

} // namespace openfranko::src::engine::effects
