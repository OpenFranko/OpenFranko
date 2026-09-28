#include "DoubleBuffer.h"

#include <cstddef>
#include <utility>

namespace openfranko::src::engine::street {
namespace {

constexpr int FIRST_DRAW = 1;
constexpr int SECOND_DRAW = 2;
constexpr int LAST_VBL = 3;

} // namespace

DoubleBuffer::DoubleBuffer(const IndexedSurface &screen)
    : m_buffers{Buffer{screen, {}}, Buffer{screen, {}}} {}

const IndexedSurface &DoubleBuffer::shown() const {
  return m_buffers[static_cast<std::size_t>(m_shown)].pixels;
}

IndexedSurface &DoubleBuffer::logic() {
  return m_buffers[static_cast<std::size_t>(m_logic)].pixels;
}

const IndexedSurface &DoubleBuffer::logic() const {
  return m_buffers[static_cast<std::size_t>(m_logic)].pixels;
}

bool DoubleBuffer::isAutobacking() const { return m_phase != 0; }

bool DoubleBuffer::isDirty(const BobLayer &bobs) const {
  return snapshot(bobs) != m_drawn;
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
  BobLayer::restore(buffer.pixels, buffer.saved);
  buffer.saved.clear();
}

void DoubleBuffer::drawBobs(const BobLayer &bobs, ImageBank &images) {
  Buffer &buffer = m_buffers[static_cast<std::size_t>(m_logic)];
  buffer.saved = bobs.drawSaving(buffer.pixels, images);
  m_drawn = snapshot(bobs);
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
    m_drawn = snapshot(bobs);
    m_op = nullptr;
    m_phase = 0;
  }
}

DoubleBuffer::Snapshot DoubleBuffer::snapshot(const BobLayer &bobs) {
  Snapshot state{};
  for (int number = 0; number < BobLayer::COUNT; ++number) {
    BobState &bob = state[static_cast<std::size_t>(number)];
    bob.active = bobs.isActive(number);
    if (bob.active) {
      bob.x = bobs.x(number);
      bob.y = bobs.y(number);
      bob.image = bobs.image(number);
    }
  }
  return state;
}

void DoubleBuffer::update(const BobLayer &bobs, ImageBank &images) {
  clearBobs();
  drawBobs(bobs, images);
  swap();
}

} // namespace openfranko::src::engine::street
