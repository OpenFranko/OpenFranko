#include "InkeyBuffer.h"

namespace openfranko::src::engine::effects {

void InkeyBuffer::press(char key) {
  m_pressed.push_back(key);
  if (m_permitted) {
    deliver();
  }
}

void InkeyBuffer::permit() {
  m_permitted = true;
  deliver();
}

void InkeyBuffer::sleep() { deliver(); }

std::optional<char> InkeyBuffer::inkey() {
  if (m_buffer.empty()) {
    return std::nullopt;
  }
  const char key = m_buffer.front();
  m_buffer.pop_front();
  return key;
}

bool InkeyBuffer::isEmpty() const { return m_buffer.empty(); }

void InkeyBuffer::deliver() {
  for (const char key : m_pressed) {
    if (m_buffer.size() < CAPACITY) {
      m_buffer.push_back(key);
    }
  }
  m_pressed.clear();
}

} // namespace openfranko::src::engine::effects
