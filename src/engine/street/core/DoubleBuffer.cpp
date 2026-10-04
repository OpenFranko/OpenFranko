#include "DoubleBuffer.h"

#include "../../../systems/graphics/Display.h"

#include <cstddef>
#include <utility>

namespace openfranko::src::engine::street::core {
namespace {

constexpr int FIRST_DRAW = 1;
constexpr int SECOND_DRAW = 2;
constexpr int LAST_VBL = 3;
constexpr uint32_t LOW_BYTE = 0xFF;
constexpr int BYTE_BITS = 8;

} // namespace

DoubleBuffer::DoubleBuffer(const IndexedSurface &screen)
    : m_buffers{Buffer{screen, {}, 0}, Buffer{screen, {}, 0}} {}

DoubleBuffer::DoubleBuffer(IndexedSurface &&screen)
    : m_buffers{Buffer{screen, {}, 0}, Buffer{std::move(screen), {}, 0}} {}

DoubleBuffer::DoubleBuffer(int width, int height)
    : m_buffers{Buffer{IndexedSurface(width, height), {}, 0},
                Buffer{IndexedSurface(width, height), {}, 0}} {}

DoubleBuffer::DoubleBuffer(const DoubleBuffer &other)
    : m_buffers(other.m_buffers), m_logic(other.m_logic),
      m_shown(other.m_shown), m_vbl(other.m_vbl), m_updates(other.m_updates),
      m_drawn(other.m_drawn), m_op(other.m_op), m_phase(other.m_phase) {
  adopt();
}

DoubleBuffer::DoubleBuffer(DoubleBuffer &&other)
    : m_buffers(std::move(other.m_buffers)), m_logic(other.m_logic),
      m_shown(other.m_shown), m_vbl(other.m_vbl), m_updates(other.m_updates),
      m_drawn(other.m_drawn), m_op(std::move(other.m_op)),
      m_phase(other.m_phase) {
  adopt();
}

DoubleBuffer &DoubleBuffer::operator=(const DoubleBuffer &other) {
  if (this != &other) {
    m_buffers = other.m_buffers;
    m_logic = other.m_logic;
    m_shown = other.m_shown;
    m_vbl = other.m_vbl;
    m_updates = other.m_updates;
    m_drawn = other.m_drawn;
    m_op = other.m_op;
    m_phase = other.m_phase;
    adopt();
  }
  return *this;
}

DoubleBuffer &DoubleBuffer::operator=(DoubleBuffer &&other) {
  if (this != &other) {
    m_buffers = std::move(other.m_buffers);
    m_logic = other.m_logic;
    m_shown = other.m_shown;
    m_vbl = other.m_vbl;
    m_updates = other.m_updates;
    m_drawn = other.m_drawn;
    m_op = std::move(other.m_op);
    m_phase = other.m_phase;
    adopt();
  }
  return *this;
}

void DoubleBuffer::adopt() {
  bakeBuffer(m_buffers[0]);
  bakeBuffer(m_buffers[1]);
  m_sprites = false;
}

void DoubleBuffer::bakeBuffer(Buffer &buffer) {
  if (buffer.sprites.empty()) {
    return;
  }
  buffer.savedCount =
      BobLayer::bake(buffer.pixels, buffer.sprites, buffer.saved);
  buffer.sprites.clear();
  buffer.version = systems::graphics::newRevision();
}

const IndexedSurface &DoubleBuffer::shown() const {
  Buffer &buffer = m_buffers[static_cast<std::size_t>(m_shown)];
  bakeBuffer(buffer);
  return buffer.pixels;
}

const IndexedSurface &DoubleBuffer::upcoming() const {
  Buffer &buffer = m_buffers[static_cast<std::size_t>(1 - m_logic)];
  bakeBuffer(buffer);
  return buffer.pixels;
}

DoubleBuffer::View DoubleBuffer::shownView() const {
  const Buffer &buffer = m_buffers[static_cast<std::size_t>(m_shown)];
  return {buffer.pixels, buffer.sprites, buffer.version};
}

DoubleBuffer::View DoubleBuffer::upcomingView() const {
  const Buffer &buffer = m_buffers[static_cast<std::size_t>(1 - m_logic)];
  return {buffer.pixels, buffer.sprites, buffer.version};
}

IndexedSurface &DoubleBuffer::logic() {
  Buffer &buffer = m_buffers[static_cast<std::size_t>(m_logic)];
  bakeBuffer(buffer);
  return buffer.pixels;
}

const IndexedSurface &DoubleBuffer::logic() const {
  Buffer &buffer = m_buffers[static_cast<std::size_t>(m_logic)];
  bakeBuffer(buffer);
  return buffer.pixels;
}

void DoubleBuffer::setSprites(bool on) {
  if (!on) {
    bake();
  }
  m_sprites = on;
}

void DoubleBuffer::bake() {
  bakeBuffer(m_buffers[0]);
  bakeBuffer(m_buffers[1]);
}

void DoubleBuffer::bakeUsing(const uint8_t *pixels) {
  for (Buffer &buffer : m_buffers) {
    for (const Sprite &sprite : buffer.sprites) {
      if (sprite.pixels == pixels) {
        bakeBuffer(buffer);
        break;
      }
    }
  }
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
      if ((bits & LOW_BYTE) == 0) {
        bits >>= BYTE_BITS - 1;
        number += BYTE_BITS - 1;
        continue;
      }
      if ((bits & 1u) == 0) {
        continue;
      }
      const BobState &drawn = m_drawn.bobs[static_cast<std::size_t>(number)];
      const amal::Object &object = bobs.placedObject(number);
      if (object.x != drawn.x || object.y != drawn.y ||
          object.image != drawn.image) {
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
  if (!buffer.sprites.empty()) {
    buffer.sprites.clear();
    buffer.version = systems::graphics::newRevision();
  }
  BobLayer::restore(buffer.pixels, buffer.saved, buffer.savedCount);
  buffer.savedCount = 0;
}

void DoubleBuffer::drawBobs(const BobLayer &bobs, ImageBank &images) {
  Buffer &buffer = m_buffers[static_cast<std::size_t>(m_logic)];
  if (m_sprites) {
    if (!buffer.sprites.empty()) {
      BobLayer::stamp(buffer.pixels, buffer.sprites);
      buffer.sprites.clear();
    }
    buffer.savedCount = 0;
    buffer.version = systems::graphics::newRevision();
    if (bobs.sprites(buffer.pixels, images, m_recording)) {
      buffer.sprites.swap(m_recording);
      snapshot(bobs, m_drawn);
      return;
    }
  }
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
      if ((bits & LOW_BYTE) == 0) {
        bits >>= BYTE_BITS - 1;
        number += BYTE_BITS - 1;
        continue;
      }
      if ((bits & 1u) == 0) {
        continue;
      }
      BobState &bob = state.bobs[static_cast<std::size_t>(number)];
      const amal::Object &object = bobs.placedObject(number);
      bob.active = true;
      bob.x = object.x;
      bob.y = object.y;
      bob.image = object.image;
    }
  }
}

void DoubleBuffer::update(const BobLayer &bobs, ImageBank &images) {
  clearBobs();
  drawBobs(bobs, images);
  swap();
}

} // namespace openfranko::src::engine::street::core
