#include "Bobs.h"

#include <algorithm>
#include <stdexcept>

namespace openfranko::src::engine::street {
namespace {

struct Shape {
  const Picture *picture = nullptr;
  uint16_t orientation = 0;
  int left = 0;
  int top = 0;
};

int hotX(const Picture &picture, uint16_t flags) {
  return (flags & ImageBank::FLIP_X) ? picture.width - picture.hotX
                                     : picture.hotX;
}

int hotY(const Picture &picture, uint16_t flags) {
  return (flags & ImageBank::FLIP_Y) ? picture.height - picture.hotY
                                     : picture.hotY;
}

bool solid(const Shape &shape, int x, int y) {
  const Picture &picture = *shape.picture;
  const int column = (shape.orientation & ImageBank::FLIP_X)
                         ? picture.width - 1 - (x - shape.left)
                         : x - shape.left;
  const int row = (shape.orientation & ImageBank::FLIP_Y)
                      ? picture.height - 1 - (y - shape.top)
                      : y - shape.top;
  return picture.at(column, row) != 0;
}

bool overlaps(const Shape &a, const Shape &b) {
  const int aRight = a.left + a.picture->width;
  const int aBottom = a.top + a.picture->height;
  const int bRight = b.left + b.picture->width;
  const int bBottom = b.top + b.picture->height;
  if (bRight <= a.left || b.left >= aRight || bBottom <= a.top ||
      b.top >= aBottom) {
    return false;
  }
  const int left = std::max(a.left, b.left);
  const int right = std::min(aRight, bRight);
  const int top = std::max(a.top, b.top);
  const int bottom = std::min(aBottom, bBottom);
  for (int y = top; y < bottom; ++y) {
    for (int x = left; x < right; ++x) {
      if (solid(a, x, y) && solid(b, x, y)) {
        return true;
      }
    }
  }
  return false;
}

} // namespace

void ImageBank::clear() { m_entries.clear(); }

void ImageBank::load(int base, const std::vector<Picture> &frames) {
  const std::size_t end = static_cast<std::size_t>(base) + frames.size();
  if (m_entries.size() < end) {
    m_entries.resize(end);
  }
  for (std::size_t i = 0; i < frames.size(); ++i) {
    Entry &entry = m_entries[static_cast<std::size_t>(base) + i];
    entry.picture = frames[i];
    entry.orientation = 0;
    entry.loaded = frames[i].width > 0 && frames[i].height > 0;
    entry.masked = true;
  }
}

const Picture *ImageBank::find(int number) const {
  if (number <= 0 || static_cast<std::size_t>(number) >= m_entries.size() ||
      !m_entries[static_cast<std::size_t>(number)].loaded) {
    return nullptr;
  }
  return &m_entries[static_cast<std::size_t>(number)].picture;
}

uint16_t ImageBank::orientation(int number) const {
  return find(number) ? m_entries[static_cast<std::size_t>(number)].orientation
                      : 0;
}

void ImageBank::orient(int number, uint16_t flags) {
  if (find(number)) {
    m_entries[static_cast<std::size_t>(number)].orientation =
        flags & (FLIP_X | FLIP_Y);
  }
}

void ImageBank::noMask(int number) {
  if (find(number)) {
    m_entries[static_cast<std::size_t>(number)].masked = false;
  }
}

bool ImageBank::isMasked(int number) const {
  return !find(number) || m_entries[static_cast<std::size_t>(number)].masked;
}

amal::Object &BobLayer::object(int number) {
  return m_bobs.at(static_cast<std::size_t>(number)).object;
}

void BobLayer::set(int number, int x, int y, int image) {
  Bob &bob = m_bobs.at(static_cast<std::size_t>(number));
  bob.active = true;
  bob.object.x = static_cast<int16_t>(x);
  bob.object.y = static_cast<int16_t>(y);
  bob.object.image = static_cast<int16_t>(image);
}

void BobLayer::setPosition(int number, int x, int y) {
  Bob &bob = m_bobs.at(static_cast<std::size_t>(number));
  bob.active = true;
  bob.object.x = static_cast<int16_t>(x);
  bob.object.y = static_cast<int16_t>(y);
}

void BobLayer::setX(int number, int x) {
  Bob &bob = m_bobs.at(static_cast<std::size_t>(number));
  bob.active = true;
  bob.object.x = static_cast<int16_t>(x);
}

void BobLayer::setImage(int number, int image) {
  Bob &bob = m_bobs.at(static_cast<std::size_t>(number));
  bob.active = true;
  bob.object.image = static_cast<int16_t>(image);
}

void BobLayer::off(int number) {
  m_bobs.at(static_cast<std::size_t>(number)).active = false;
}

void BobLayer::offAll() {
  for (Bob &bob : m_bobs) {
    bob.active = false;
  }
}

bool BobLayer::isActive(int number) const {
  return m_bobs.at(static_cast<std::size_t>(number)).active;
}

int16_t BobLayer::x(int number) const {
  return m_bobs.at(static_cast<std::size_t>(number)).object.x;
}

int16_t BobLayer::y(int number) const {
  return m_bobs.at(static_cast<std::size_t>(number)).object.y;
}

int16_t BobLayer::image(int number) const {
  return m_bobs.at(static_cast<std::size_t>(number)).object.image;
}

bool BobLayer::collide(int number, const ImageBank &images, int first,
                       int last) {
  m_collisions.fill(false);
  const auto shapeOf = [&images](const Bob &bob, Shape &shape) {
    if (!bob.active) {
      return false;
    }
    const int image =
        static_cast<uint16_t>(bob.object.image) & ImageBank::NUMBER_MASK;
    shape.picture = images.find(image);
    if (!shape.picture || !images.isMasked(image)) {
      return false;
    }
    shape.orientation = images.orientation(image);
    shape.left = bob.object.x - hotX(*shape.picture, shape.orientation);
    shape.top = bob.object.y - hotY(*shape.picture, shape.orientation);
    return true;
  };

  Shape tested;
  if (!shapeOf(m_bobs.at(static_cast<std::size_t>(number)), tested)) {
    return false;
  }
  bool any = false;
  for (int other = std::max(first, 0); other <= last && other < COUNT;
       ++other) {
    Shape shape;
    if (other == number ||
        !shapeOf(m_bobs[static_cast<std::size_t>(other)], shape)) {
      continue;
    }
    if (overlaps(tested, shape)) {
      m_collisions[static_cast<std::size_t>(other)] = true;
      any = true;
    }
  }
  return any;
}

bool BobLayer::collided(int number) const {
  return m_collisions.at(static_cast<std::size_t>(number));
}

void BobLayer::draw(IndexedSurface &surface, ImageBank &images) const {
  std::vector<int> order;
  for (int number = 0; number < COUNT; ++number) {
    if (m_bobs[static_cast<std::size_t>(number)].active) {
      order.push_back(number);
    }
  }
  std::stable_sort(order.begin(), order.end(), [this](int a, int b) {
    const amal::Object &first = m_bobs[static_cast<std::size_t>(a)].object;
    const amal::Object &second = m_bobs[static_cast<std::size_t>(b)].object;
    return first.y != second.y ? first.y < second.y : first.x < second.x;
  });

  for (int number : order) {
    const amal::Object &bob = m_bobs[static_cast<std::size_t>(number)].object;
    const uint16_t image = static_cast<uint16_t>(bob.image);
    const int index = image & ImageBank::NUMBER_MASK;
    const Picture *picture = images.find(index);
    if (!picture) {
      continue;
    }
    const uint16_t flags = image & (ImageBank::FLIP_X | ImageBank::FLIP_Y);
    const int left = bob.x - hotX(*picture, flags);
    const int top = bob.y - hotY(*picture, flags);
    if (!surface.intersects(left, top, picture->width, picture->height)) {
      continue;
    }
    images.orient(index, flags);
    surface.draw(*picture, left, top, flags & ImageBank::FLIP_X,
                 flags & ImageBank::FLIP_Y, !images.isMasked(index));
  }
}

bool BobLayer::paste(IndexedSurface &surface, ImageBank &images, int x, int y,
                     int image) {
  const uint16_t word = static_cast<uint16_t>(image);
  const int index = word & ImageBank::NUMBER_MASK;
  const Picture *picture = images.find(index);
  if (!picture) {
    return false;
  }
  const uint16_t flags = word & (ImageBank::FLIP_X | ImageBank::FLIP_Y);
  images.orient(index, flags);
  if (!surface.intersects(x, y, picture->width, picture->height)) {
    return false;
  }
  surface.draw(*picture, x, y, flags & ImageBank::FLIP_X,
               flags & ImageBank::FLIP_Y, !images.isMasked(index));
  return true;
}

} // namespace openfranko::src::engine::street
