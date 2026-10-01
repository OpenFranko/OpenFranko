#include "Bobs.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace openfranko::src::engine::street::core {
namespace {

struct Shape {
  ImageBank::Mask mask;
  int left = 0;
  int top = 0;
};

struct RowCursor {
  const RowSpan *span = nullptr;
  int spanStep = 1;
  const uint8_t *row = nullptr;
  int rowStep = 0;
  int origin = 0;
};

int hotX(const Picture &picture, uint16_t flags) {
  return (flags & ImageBank::FLIP_X) ? picture.width - picture.hotX
                                     : picture.hotX;
}

constexpr int WORD_PIXELS = 16;

int wordStart(int x) {
  return x >= 0 ? x & ~(WORD_PIXELS - 1)
                : -((-x + WORD_PIXELS - 1) & ~(WORD_PIXELS - 1));
}

int hotY(const Picture &picture, uint16_t flags) {
  return (flags & ImageBank::FLIP_Y) ? picture.height - picture.hotY
                                     : picture.hotY;
}

bool isOpaque(uint8_t pixel) { return pixel != 0; }

std::vector<RowSpan> rowSpans(const Picture &picture) {
  std::vector<RowSpan> rows(static_cast<std::size_t>(picture.height));
  const uint8_t *row = picture.pixels.data();
  for (RowSpan &span : rows) {
    const uint8_t *end = row + picture.width;
    const uint8_t *first = std::find_if(row, end, isOpaque);
    if (first != end) {
      const auto last =
          std::find_if(std::make_reverse_iterator(end),
                       std::make_reverse_iterator(first), isOpaque);
      span.first = static_cast<int>(first - row);
      span.last = static_cast<int>(last.base() - row) - 1;
    }
    row = end;
  }
  return rows;
}

MaskBox maskBox(const std::vector<RowSpan> &rows) {
  MaskBox box;
  bool found = false;
  for (std::size_t row = 0; row < rows.size(); ++row) {
    const RowSpan &span = rows[row];
    if (span.first > span.last) {
      continue;
    }
    const int y = static_cast<int>(row);
    if (!found) {
      box = MaskBox{span.first, y, span.last + 1, y + 1};
      found = true;
      continue;
    }
    box.left = std::min(box.left, span.first);
    box.right = std::max(box.right, span.last + 1);
    box.bottom = y + 1;
  }
  return box;
}

MaskBox screenBox(const Shape &shape) {
  const Picture &picture = *shape.mask.picture;
  const MaskBox &box = shape.mask.box;
  MaskBox screen;
  if (shape.mask.orientation & ImageBank::FLIP_X) {
    screen.left = shape.left + picture.width - box.right;
    screen.right = shape.left + picture.width - box.left;
  } else {
    screen.left = shape.left + box.left;
    screen.right = shape.left + box.right;
  }
  if (shape.mask.orientation & ImageBank::FLIP_Y) {
    screen.top = shape.top + picture.height - box.bottom;
    screen.bottom = shape.top + picture.height - box.top;
  } else {
    screen.top = shape.top + box.top;
    screen.bottom = shape.top + box.bottom;
  }
  return screen;
}

RowCursor rowCursor(const Shape &shape, int top) {
  const Picture &picture = *shape.mask.picture;
  const bool flipY = (shape.mask.orientation & ImageBank::FLIP_Y) != 0;
  const int row =
      flipY ? picture.height - 1 - (top - shape.top) : top - shape.top;
  RowCursor cursor;
  cursor.span = shape.mask.rows + row;
  cursor.spanStep = flipY ? -1 : 1;
  cursor.row = picture.pixels.data() + row * picture.width;
  cursor.rowStep = flipY ? -picture.width : picture.width;
  cursor.origin = (shape.mask.orientation & ImageBank::FLIP_X)
                      ? shape.left + picture.width - 1
                      : shape.left;
  return cursor;
}

template <bool FLIP_X> int solidFrom(const RowCursor &cursor) {
  return FLIP_X ? cursor.origin - cursor.span->last
                : cursor.origin + cursor.span->first;
}

template <bool FLIP_X> int solidTo(const RowCursor &cursor) {
  return FLIP_X ? cursor.origin - cursor.span->first
                : cursor.origin + cursor.span->last;
}

template <bool FLIP_X> const uint8_t *pixelAt(const RowCursor &cursor, int x) {
  return FLIP_X ? cursor.row + (cursor.origin - x)
                : cursor.row + (x - cursor.origin);
}

void nextRow(RowCursor &cursor) {
  cursor.span += cursor.spanStep;
  cursor.row += cursor.rowStep;
}

template <bool FIRST_FLIP_X, bool SECOND_FLIP_X>
bool solidRowsMeet(RowCursor first, RowCursor second, int left, int right,
                   int rows) {
  constexpr int FIRST_STEP = FIRST_FLIP_X ? -1 : 1;
  constexpr int SECOND_STEP = SECOND_FLIP_X ? -1 : 1;
  for (;;) {
    const int from = std::max(left, std::max(solidFrom<FIRST_FLIP_X>(first),
                                             solidFrom<SECOND_FLIP_X>(second)));
    const int to = std::min(right, std::min(solidTo<FIRST_FLIP_X>(first),
                                            solidTo<SECOND_FLIP_X>(second)));
    if (from <= to) {
      const uint8_t *firstPixel = pixelAt<FIRST_FLIP_X>(first, from);
      const uint8_t *secondPixel = pixelAt<SECOND_FLIP_X>(second, from);
      for (int x = from; x <= to; ++x) {
        if (*firstPixel != 0 && *secondPixel != 0) {
          return true;
        }
        firstPixel += FIRST_STEP;
        secondPixel += SECOND_STEP;
      }
    }
    if (--rows == 0) {
      return false;
    }
    nextRow(first);
    nextRow(second);
  }
}

bool solidPixelsMeet(const Shape &a, const Shape &b, const MaskBox &area) {
  const MaskBox firstBox = screenBox(a);
  const MaskBox secondBox = screenBox(b);
  const int left = std::max(area.left, std::max(firstBox.left, secondBox.left));
  const int right =
      std::min(area.right, std::min(firstBox.right, secondBox.right));
  const int top = std::max(area.top, std::max(firstBox.top, secondBox.top));
  const int bottom =
      std::min(area.bottom, std::min(firstBox.bottom, secondBox.bottom));
  if (left >= right || top >= bottom) {
    return false;
  }
  const RowCursor first = rowCursor(a, top);
  const RowCursor second = rowCursor(b, top);
  const bool firstFlipX = (a.mask.orientation & ImageBank::FLIP_X) != 0;
  const bool secondFlipX = (b.mask.orientation & ImageBank::FLIP_X) != 0;
  const int rows = bottom - top;
  if (firstFlipX) {
    return secondFlipX
               ? solidRowsMeet<true, true>(first, second, left, right - 1, rows)
               : solidRowsMeet<true, false>(first, second, left, right - 1,
                                            rows);
  }
  return secondFlipX
             ? solidRowsMeet<false, true>(first, second, left, right - 1, rows)
             : solidRowsMeet<false, false>(first, second, left, right - 1,
                                           rows);
}

MaskBox blitBox(const Shape &shape) {
  const Picture &picture = *shape.mask.picture;
  const int words = (picture.width + WORD_PIXELS - 1) / WORD_PIXELS;
  return MaskBox{shape.left, shape.top, shape.left + words * WORD_PIXELS,
                 shape.top + picture.height};
}

bool overlaps(const Shape &tested, const Shape &other) {
  const MaskBox testedBox = blitBox(tested);
  const MaskBox otherBox = blitBox(other);
  const MaskBox shared{std::max(testedBox.left, otherBox.left),
                       std::max(testedBox.top, otherBox.top),
                       std::min(testedBox.right, otherBox.right),
                       std::min(testedBox.bottom, otherBox.bottom)};
  if (shared.left >= shared.right || shared.top >= shared.bottom) {
    return false;
  }
  if (solidPixelsMeet(tested, other, shared)) {
    return true;
  }
  const bool testedOnRight = tested.left >= other.left;
  const Shape &left = testedOnRight ? other : tested;
  const Shape &right = testedOnRight ? tested : other;
  const MaskBox &leftBox = testedOnRight ? otherBox : testedBox;
  const MaskBox &rightBox = testedOnRight ? testedBox : otherBox;
  const int shift = (right.left - left.left) & (WORD_PIXELS - 1);
  if (shift == 0 || rightBox.right >= leftBox.right) {
    return false;
  }
  Shape spilled = right;
  spilled.left = rightBox.right;
  spilled.top = right.top - 1;
  const MaskBox strip{rightBox.right, shared.top,
                      rightBox.right + WORD_PIXELS - shift, shared.bottom};
  return solidPixelsMeet(left, spilled, strip);
}

} // namespace

void ImageBank::clear() {
  m_entries.clear();
  m_outlines.clear();
}

void ImageBank::load(int base, std::vector<Picture> frames) {
  const std::size_t end = static_cast<std::size_t>(base) + frames.size();
  if (m_entries.size() < end) {
    m_entries.resize(end);
    m_outlines.resize(end);
  }
  for (std::size_t i = 0; i < frames.size(); ++i) {
    Entry &entry = m_entries[static_cast<std::size_t>(base) + i];
    Outline &outline = m_outlines[static_cast<std::size_t>(base) + i];
    outline.rows = rowSpans(frames[i]);
    outline.box = maskBox(outline.rows);
    entry.loaded = frames[i].width > 0 && frames[i].height > 0;
    entry.picture = std::move(frames[i]);
    entry.orientation = 0;
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

ImageBank::Mask ImageBank::mask(int number) const {
  if (!find(number)) {
    return {};
  }
  const Entry &entry = m_entries[static_cast<std::size_t>(number)];
  if (!entry.masked) {
    return {};
  }
  const Outline &outline = m_outlines[static_cast<std::size_t>(number)];
  return Mask{&entry.picture, outline.rows.data(), outline.box,
              entry.orientation};
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

bool BobLayer::collide(int number, const ImageBank &images, int first,
                       int last) {
  m_collisions.fill(false);
  const auto shapeOf = [&images](const Bob &bob, Shape &shape) {
    if (!bob.active) {
      return false;
    }
    const int image =
        static_cast<uint16_t>(bob.object.image) & ImageBank::NUMBER_MASK;
    shape.mask = images.mask(image);
    if (!shape.mask.picture) {
      return false;
    }
    shape.left =
        bob.object.x - hotX(*shape.mask.picture, shape.mask.orientation);
    shape.top =
        bob.object.y - hotY(*shape.mask.picture, shape.mask.orientation);
    return true;
  };

  Shape tested;
  if (!shapeOf(m_bobs.at(static_cast<std::size_t>(number)), tested)) {
    return false;
  }
  bool any = false;
  const int end = std::min(last, BOBS - 1);
  for (int other = std::max(first, 0); other <= end; ++other) {
    const Bob &bob = m_bobs[static_cast<std::size_t>(other)];
    if (!bob.active || other == number) {
      continue;
    }
    Shape shape;
    if (shapeOf(bob, shape) && overlaps(tested, shape)) {
      m_collisions[static_cast<std::size_t>(other)] = true;
      any = true;
    }
  }
  return any;
}

bool BobLayer::collided(int number) const {
  return m_collisions.at(static_cast<std::size_t>(number));
}

const std::vector<BobLayer::Placement> &
BobLayer::placements(const IndexedSurface &surface,
                     const ImageBank &images) const {
  m_order.clear();
  for (int number = 0; number < BOBS; ++number) {
    const Bob &bob = m_bobs[static_cast<std::size_t>(number)];
    if (!bob.active) {
      continue;
    }
    std::size_t at = m_order.size();
    m_order.push_back(number);
    while (at > 0) {
      const amal::Object &before =
          m_bobs[static_cast<std::size_t>(m_order[at - 1])].object;
      if (before.y < bob.object.y ||
          (before.y == bob.object.y && before.x <= bob.object.x)) {
        break;
      }
      m_order[at] = m_order[at - 1];
      --at;
    }
    m_order[at] = number;
  }

  m_placed.clear();
  for (int number : m_order) {
    const amal::Object &bob = m_bobs[static_cast<std::size_t>(number)].object;
    const uint16_t image = static_cast<uint16_t>(bob.image);
    const Picture *picture = images.find(image & ImageBank::NUMBER_MASK);
    if (!picture) {
      continue;
    }
    const uint16_t flags = image & (ImageBank::FLIP_X | ImageBank::FLIP_Y);
    const int left = bob.x - hotX(*picture, flags);
    const int top = bob.y - hotY(*picture, flags);
    if (surface.intersects(left, top, picture->width, picture->height)) {
      m_placed.push_back({number, picture, flags, left, top});
    }
  }
  return m_placed;
}

void BobLayer::draw(IndexedSurface &surface, ImageBank &images) const {
  drawPlaced(surface, images, placements(surface, images));
}

void BobLayer::drawPlaced(IndexedSurface &surface, ImageBank &images,
                          const std::vector<Placement> &placedBobs) const {
  for (const Placement &placed : placedBobs) {
    const int bob = placed.number;
    const int index = static_cast<uint16_t>(
                          m_bobs[static_cast<std::size_t>(bob)].object.image) &
                      ImageBank::NUMBER_MASK;
    images.orient(index, placed.flags);
    surface.draw(*placed.picture, placed.left, placed.top,
                 placed.flags & ImageBank::FLIP_X,
                 placed.flags & ImageBank::FLIP_Y, !images.isMasked(index));
  }
}

std::size_t BobLayer::drawSaving(IndexedSurface &surface, ImageBank &images,
                                 std::vector<SavedArea> &saved) const {
  std::size_t count = 0;
  const std::vector<Placement> &placedBobs = placements(surface, images);
  for (const Placement &placed : placedBobs) {
    const int words = (placed.picture->width + WORD_PIXELS - 1) / WORD_PIXELS +
                      ((placed.left & (WORD_PIXELS - 1)) != 0 ? 1 : 0);
    const int start = wordStart(placed.left);
    const int x1 = std::max(start, 0);
    const int x2 = std::min(start + words * WORD_PIXELS, surface.width());
    const int y1 = std::max(placed.top, 0);
    const int y2 =
        std::min(placed.top + placed.picture->height, surface.height());
    if (x1 >= x2 || y1 >= y2) {
      continue;
    }
    if (count == saved.size()) {
      saved.emplace_back();
    }
    SavedArea &area = saved[count++];
    area.left = x1;
    area.top = y1;
    area.pixels.reshape(x2 - x1, y2 - y1);
    area.pixels.copy(surface, x1, y1, x2, y2, 0, 0);
  }
  drawPlaced(surface, images, placedBobs);
  return count;
}

void BobLayer::restore(IndexedSurface &surface,
                       const std::vector<SavedArea> &saved, std::size_t count) {
  for (std::size_t index = 0; index < count; ++index) {
    const SavedArea &area = saved[index];
    surface.copy(area.pixels, 0, 0, area.pixels.width(), area.pixels.height(),
                 area.left, area.top);
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

} // namespace openfranko::src::engine::street::core
