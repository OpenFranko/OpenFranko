#include "DoubleBuffer.h"

#include <cstddef>
#include <utility>

namespace openfranko::src::engine::street::core {
namespace {

constexpr int FIRST_DRAW = 1;
constexpr int SECOND_DRAW = 2;
constexpr int LAST_VBL = 3;

} // namespace

DoubleBuffer::DoubleBuffer(const IndexedSurface &screen)
    : m_buffers{Buffer{screen, {}, 0}, Buffer{screen, {}, 0}} {}

DoubleBuffer::DoubleBuffer(int width, int height)
    : m_buffers{Buffer{IndexedSurface(width, height), {}, 0},
                Buffer{IndexedSurface(width, height), {}, 0}} {}

const IndexedSurface &DoubleBuffer::shown() const {
  return m_buffers[static_cast<std::size_t>(m_shown)].pixels;
}

const IndexedSurface &DoubleBuffer::upcoming() const {
  return m_buffers[static_cast<std::size_t>(1 - m_logic)].pixels;
}

IndexedSurface &DoubleBuffer::logic() {
  return m_buffers[static_cast<std::size_t>(m_logic)].pixels;
}

const IndexedSurface &DoubleBuffer::logic() const {
  return m_buffers[static_cast<std::size_t>(m_logic)].pixels;
}

bool DoubleBuffer::isAutobacking() const { return m_phase != 0; }

bool DoubleBuffer::isDirty(const BobLayer &bobs) const {
  for (int word = 0; word < BobLayer::MASK_WORDS; ++word) {
    const uint32_t active = bobs.activeBits(word);
    if (active != m_drawn.active[static_cast<std::size_t>(word)]) {
      return true;
    }
    int number = word * BobLayer::MASK_BITS;
    for (uint32_t bits = active; bits != 0; bits >>= 1, ++number) {
      if ((bits & 1u) == 0) {
        continue;
      }
      const BobState &drawn = m_drawn.bobs[static_cast<std::size_t>(number)];
      if (bobs.x(number) != drawn.x || bobs.y(number) != drawn.y ||
          bobs.image(number) != drawn.image) {
        return true;
      }
    }
  }
  return false;
}

void DoubleBuffer::vbl() {
  m_shown = 1 - m_logic;
  m_vbl = true;
}

bool DoubleBuffer::test(const BobLayer &bobs, ImageBank &images) {
  if (!m_vbl) {
    return false;
  }
  m_vbl = false;
  if (m_updates && isDirty(bobs)) {
    update(bobs, images);
  }
  return true;
}

void DoubleBuffer::setUpdates(bool on) { m_updates = on; }

void DoubleBuffer::clearBobs() {
  Buffer &buffer = m_buffers[static_cast<std::size_t>(m_logic)];
  BobLayer::restore(buffer.pixels, buffer.saved, buffer.savedCount);
  buffer.savedCount = 0;
}

void DoubleBuffer::drawBobs(const BobLayer &bobs, ImageBank &images) {
  Buffer &buffer = m_buffers[static_cast<std::size_t>(m_logic)];
  buffer.savedCount = bobs.drawSaving(buffer.pixels, images, buffer.saved);
  snapshot(bobs, m_drawn);
}

void DoubleBuffer::swap() { m_logic = 1 - m_logic; }

void DoubleBuffer::autoback(Op op) {
  m_op = std::move(op);
  m_phase = FIRST_DRAW;
}

void DoubleBuffer::autobackStep(const BobLayer &bobs, ImageBank &images) {
  if (m_phase == FIRST_DRAW || m_phase == SECOND_DRAW) {
    clearBobs();
    m_op(logic());
    drawBobs(bobs, images);
    swap();
    ++m_phase;
    return;
  }
  if (m_phase == LAST_VBL) {
    snapshot(bobs, m_drawn);
    m_op = nullptr;
    m_phase = 0;
  }
}

void DoubleBuffer::snapshot(const BobLayer &bobs, Snapshot &state) {
  for (int word = 0; word < BobLayer::MASK_WORDS; ++word) {
    const uint32_t active = bobs.activeBits(word);
    state.active[static_cast<std::size_t>(word)] = active;
    int number = word * BobLayer::MASK_BITS;
    for (uint32_t bits = active; bits != 0; bits >>= 1, ++number) {
      if ((bits & 1u) == 0) {
        continue;
      }
      BobState &bob = state.bobs[static_cast<std::size_t>(number)];
      bob.active = true;
      bob.x = bobs.x(number);
      bob.y = bobs.y(number);
      bob.image = bobs.image(number);
    }
  }
}

void DoubleBuffer::update(const BobLayer &bobs, ImageBank &images) {
  clearBobs();
  drawBobs(bobs, images);
  swap();
}

} // namespace openfranko::src::engine::street::core
