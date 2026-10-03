#include "Bobs.h"

#include "../../../systems/Multiply.h"
#include "../../../systems/graphics/PixelOps.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace openfranko::src::engine::street::core {
namespace {

constexpr int SIGN_BIT = 0x8000;
constexpr std::size_t MIRROR_BYTES = 65536;
constexpr int16_t NO_MIRROR = -1;

struct Shape {
  ImageBank::Mask mask;
  int left = 0;
  int top = 0;
  MaskBox solid;
};

struct RowCursor {
  const RowSpan *span = nullptr;
  const RowSpan *band = nullptr;
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

constexpr RowSpan EMPTY_ROW{std::numeric_limits<int16_t>::max(),
                            std::numeric_limits<int16_t>::min()};

void rowSpans(const Picture &picture, RowSpan *rows) {
  const uint8_t *row = picture.pixels.data();
  for (RowSpan *span = rows; span != rows + picture.height; ++span) {
    *span = EMPTY_ROW;
    const uint8_t *end = row + picture.width;
    const uint8_t *first = row;
    while (first != end && !isOpaque(*first)) {
      ++first;
    }
    if (first != end) {
      const uint8_t *last = end - 1;
      while (!isOpaque(*last)) {
        --last;
      }
      span->first = static_cast<int16_t>(first - row);
      span->last = static_cast<int16_t>(last - row);
    }
    row = end;
  }
}

void widen(RowSpan &span, const RowSpan &other) {
  if (other.first < span.first) {
    span.first = other.first;
  }
  if (other.last > span.last) {
    span.last = other.last;
  }
}

void rowBands(const RowSpan *rows, std::size_t height, RowSpan *bands) {
  constexpr std::size_t HALF_BAND = ImageBank::BAND_ROWS / 2;
  std::copy(rows, rows + height, bands);
  for (std::size_t row = 1; row < height; ++row) {
    widen(bands[row - 1], bands[row]);
  }
  for (std::size_t row = HALF_BAND; row < height; ++row) {
    widen(bands[row - HALF_BAND], bands[row]);
  }
}

MaskBox maskBox(const RowSpan *rows, std::size_t height) {
  MaskBox box;
  bool found = false;
  for (std::size_t row = 0; row < height; ++row) {
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
    box.left = std::min<int>(box.left, span.first);
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

Shape shapeAt(const ImageBank::Mask &mask, int left, int top) {
  Shape shape{mask, left, top, MaskBox{}};
  shape.solid = screenBox(shape);
  return shape;
}

RowCursor rowCursor(const Shape &shape, int top) {
  const Picture &picture = *shape.mask.picture;
  const bool flipY = (shape.mask.orientation & ImageBank::FLIP_Y) != 0;
  const int row =
      flipY ? picture.height - 1 - (top - shape.top) : top - shape.top;
  RowCursor cursor;
  cursor.span = shape.mask.rows + row;
  cursor.band = flipY ? nullptr : shape.mask.bands + row;
  cursor.spanStep = flipY ? -1 : 1;
  cursor.row = picture.pixels.data() +
               systems::multiplySigned16(static_cast<int16_t>(row),
                                         static_cast<int16_t>(picture.width));
  cursor.rowStep = flipY ? -picture.width : picture.width;
  cursor.origin = (shape.mask.orientation & ImageBank::FLIP_X)
                      ? shape.left + picture.width - 1
                      : shape.left;
  return cursor;
}

template <bool FLIP_X> int solidFrom(int origin, const RowSpan &span) {
  return FLIP_X ? origin - span.last : origin + span.first;
}

template <bool FLIP_X> int solidTo(int origin, const RowSpan &span) {
  return FLIP_X ? origin - span.first : origin + span.last;
}

template <bool FLIP_X> const uint8_t *pixelAt(const RowCursor &cursor, int x) {
  return FLIP_X ? cursor.row + (cursor.origin - x)
                : cursor.row + (x - cursor.origin);
}

void nextRow(RowCursor &cursor) {
  cursor.span += cursor.spanStep;
  cursor.row += cursor.rowStep;
}

void nextBandedRow(RowCursor &cursor) {
  ++cursor.span;
  ++cursor.band;
  cursor.row += cursor.rowStep;
}

void skipBand(RowCursor &cursor) {
  cursor.span += ImageBank::BAND_ROWS;
  cursor.band += ImageBank::BAND_ROWS;
  cursor.row += cursor.rowStep * ImageBank::BAND_ROWS;
}

template <bool FIRST_FLIP_X, bool SECOND_FLIP_X>
bool spansMeet(const RowCursor &first, const RowSpan &firstSpan,
               const RowCursor &second, const RowSpan &secondSpan, int left,
               int right, int &from, int &to) {
  from = std::max(
      left, std::max(solidFrom<FIRST_FLIP_X>(first.origin, firstSpan),
                     solidFrom<SECOND_FLIP_X>(second.origin, secondSpan)));
  to = std::min(right,
                std::min(solidTo<FIRST_FLIP_X>(first.origin, firstSpan),
                         solidTo<SECOND_FLIP_X>(second.origin, secondSpan)));
  return from <= to;
}

template <bool FIRST_FLIP_X, bool SECOND_FLIP_X>
bool rowMeets(const RowCursor &first, const RowCursor &second, int left,
              int right) {
  constexpr int FIRST_STEP = FIRST_FLIP_X ? -1 : 1;
  constexpr int SECOND_STEP = SECOND_FLIP_X ? -1 : 1;
  int from = 0;
  int to = 0;
  if (!spansMeet<FIRST_FLIP_X, SECOND_FLIP_X>(
          first, *first.span, second, *second.span, left, right, from, to)) {
    return false;
  }
  const uint8_t *firstPixel = pixelAt<FIRST_FLIP_X>(first, from);
  const uint8_t *secondPixel = pixelAt<SECOND_FLIP_X>(second, from);
  for (int x = from; x <= to; ++x) {
    if (*firstPixel != 0 && *secondPixel != 0) {
      return true;
    }
    firstPixel += FIRST_STEP;
    secondPixel += SECOND_STEP;
  }
  return false;
}

template <bool FIRST_FLIP_X, bool SECOND_FLIP_X>
bool solidRowsMeet(RowCursor first, RowCursor second, int left, int right,
                   int rows) {
  if (!first.band || !second.band) {
    for (;;) {
      if (rowMeets<FIRST_FLIP_X, SECOND_FLIP_X>(first, second, left, right)) {
        return true;
      }
      if (--rows == 0) {
        return false;
      }
      nextRow(first);
      nextRow(second);
    }
  }
  for (;;) {
    int from = 0;
    int to = 0;
    if (!spansMeet<FIRST_FLIP_X, SECOND_FLIP_X>(
            first, *first.band, second, *second.band, left, right, from, to)) {
      if (rows <= ImageBank::BAND_ROWS) {
        return false;
      }
      rows -= ImageBank::BAND_ROWS;
      skipBand(first);
      skipBand(second);
      continue;
    }
    for (int count = std::min(rows, ImageBank::BAND_ROWS); count > 0; --count) {
      if (rowMeets<FIRST_FLIP_X, SECOND_FLIP_X>(first, second, left, right)) {
        return true;
      }
      if (--rows == 0) {
        return false;
      }
      nextBandedRow(first);
      nextBandedRow(second);
    }
  }
}

bool solidPixelsMeet(const Shape &a, const Shape &b, const MaskBox &area) {
  const MaskBox &firstBox = a.solid;
  const MaskBox &secondBox = b.solid;
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

bool overlaps(const Shape &tested, const MaskBox &testedBox, const Shape &other,
              const MaskBox &otherBox) {
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
  const int moved = spilled.left - right.left;
  spilled.solid = MaskBox{right.solid.left + moved, right.solid.top - 1,
                          right.solid.right + moved, right.solid.bottom - 1};
  const MaskBox strip{rightBox.right, shared.top,
                      rightBox.right + WORD_PIXELS - shift, shared.bottom};
  return solidPixelsMeet(left, spilled, strip);
}

constexpr uint32_t LOW_BYTE = 0xFF;
constexpr int BYTE_BITS = 8;

uint32_t rangeBits(int from, int to) {
  if (from > to || to < 0 || from >= BobLayer::MASK_BITS) {
    return 0;
  }
  const uint32_t below =
      from <= 0 ? 0u : (1u << static_cast<unsigned>(from)) - 1u;
  const uint32_t upTo = to >= BobLayer::MASK_BITS - 1
                            ? ~0u
                            : (1u << static_cast<unsigned>(to + 1)) - 1u;
  return upTo & ~below;
}

} // namespace

void ImageBank::clear() {
  systems::graphics::pixels::finish();
  for (Entry &entry : m_entries) {
    retire(std::move(entry.picture.pixels));
  }
  for (Mirror &mirror : m_mirrors) {
    retire(std::move(mirror.picture.pixels));
  }
  m_entries.clear();
  m_outlines.clear();
  m_boxes.clear();
  m_mirrors.clear();
  m_mirrorSlots.clear();
  m_mirrorBytes = 0;
}

void ImageBank::load(int base, std::vector<Picture> frames) {
  systems::graphics::pixels::finish();
  grow(static_cast<std::size_t>(base) + frames.size());
  for (std::size_t i = 0; i < frames.size(); ++i) {
    store(static_cast<std::size_t>(base) + i, std::move(frames[i]));
  }
}

void ImageBank::load(int number, Picture picture) {
  systems::graphics::pixels::finish();
  grow(static_cast<std::size_t>(number) + 1);
  store(static_cast<std::size_t>(number), std::move(picture));
}

void ImageBank::grow(std::size_t end) {
  if (m_entries.size() < end) {
    m_entries.resize(end);
    m_outlines.resize(end);
    m_boxes.resize(end);
    m_mirrorSlots.resize(end, NO_MIRROR);
  }
}

void ImageBank::store(std::size_t number, Picture &&picture) {
  forgetMirror(static_cast<int>(number));
  Entry &entry = m_entries[number];
  Outline &outline = m_outlines[number];
  const std::size_t height =
      static_cast<std::size_t>(std::max(picture.height, 0));
  if (outline.capacity < height) {
    outline.spans.reset(new RowSpan[2 * height]);
    outline.capacity = height;
  }
  RowSpan *rows = outline.spans.get();
  RowSpan *bands = rows + outline.capacity;
  if (!systems::graphics::pixels::outline(picture.pixels.data(), picture.width,
                                          picture.height, rows, bands,
                                          outline.box)) {
    rowSpans(picture, rows);
    rowBands(rows, height, bands);
    outline.box = maskBox(rows, height);
  }
  entry.loaded = picture.width > 0 && picture.height > 0;
  retire(std::move(entry.picture.pixels));
  entry.picture = std::move(picture);
  entry.orientation = 0;
  entry.masked = true;
  refreshBox(number);
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
  return Mask{&entry.picture, outline.spans.get(),
              outline.spans.get() + outline.capacity, outline.box,
              entry.orientation};
}

uint16_t ImageBank::orientation(int number) const {
  return find(number) ? m_entries[static_cast<std::size_t>(number)].orientation
                      : 0;
}

void ImageBank::orient(int number, uint16_t flags) {
  if (!find(number)) {
    return;
  }
  uint16_t &orientation =
      m_entries[static_cast<std::size_t>(number)].orientation;
  const uint16_t wanted = flags & (FLIP_X | FLIP_Y);
  if (orientation != wanted) {
    orientation = wanted;
    refreshBox(static_cast<std::size_t>(number));
  }
}

void ImageBank::noMask(int number) {
  if (find(number)) {
    m_entries[static_cast<std::size_t>(number)].masked = false;
    refreshBox(static_cast<std::size_t>(number));
  }
}

void ImageBank::refreshBox(std::size_t number) {
  const Entry &entry = m_entries[number];
  Box &box = m_boxes[number];
  if (!entry.loaded || !entry.masked) {
    box = Box{};
    return;
  }
  const Picture &picture = entry.picture;
  box.hotX = static_cast<int16_t>(hotX(picture, entry.orientation));
  box.hotY = static_cast<int16_t>(hotY(picture, entry.orientation));
  box.width = static_cast<int16_t>((picture.width + WORD_PIXELS - 1) /
                                   WORD_PIXELS * WORD_PIXELS);
  box.height = static_cast<int16_t>(picture.height);
}

const Picture *ImageBank::mirrored(int number) {
  const Picture *original = find(number);
  if (!original) {
    return nullptr;
  }
  ++m_mirrorUses;
  const int16_t found = m_mirrorSlots[static_cast<std::size_t>(number)];
  if (found != NO_MIRROR) {
    Mirror &mirror = m_mirrors[static_cast<std::size_t>(found)];
    mirror.used = m_mirrorUses;
    mirror.pass = m_pass;
    return &mirror.picture;
  }
  const std::size_t bytes = original->pixels.size();
  if (bytes == 0 || bytes > MIRROR_BYTES) {
    return nullptr;
  }
  if (m_mirrorBytes + bytes > MIRROR_BYTES) {
    if (!m_beforeRetire) {
      systems::graphics::pixels::finish();
    }
    while (m_mirrorBytes + bytes > MIRROR_BYTES) {
      std::size_t oldest = m_mirrors.size();
      for (std::size_t slot = 0; slot < m_mirrors.size(); ++slot) {
        const Mirror &mirror = m_mirrors[slot];
        if (mirror.pass != m_pass && (oldest == m_mirrors.size() ||
                                      mirror.used < m_mirrors[oldest].used)) {
          oldest = slot;
        }
      }
      if (oldest == m_mirrors.size()) {
        return nullptr;
      }
      dropMirror(oldest);
    }
  }
  Mirror mirror;
  mirror.number = number;
  mirror.used = m_mirrorUses;
  mirror.pass = m_pass;
  mirror.picture.width = original->width;
  mirror.picture.height = original->height;
  mirror.picture.hotX = original->hotX;
  mirror.picture.hotY = original->hotY;
  mirror.picture.pixels.resize(bytes);
  systems::graphics::pixels::draw(
      {original->pixels.data(), original->width},
      {mirror.picture.pixels.data(), original->width}, original->width,
      original->height, false, true);
  m_mirrorBytes += bytes;
  m_mirrorSlots[static_cast<std::size_t>(number)] =
      static_cast<int16_t>(m_mirrors.size());
  m_mirrors.push_back(std::move(mirror));
  return &m_mirrors.back().picture;
}

const Picture *ImageBank::spriteImage(int number, uint16_t flags) {
  if (number <= 0 || static_cast<std::size_t>(number) >= m_entries.size()) {
    return nullptr;
  }
  Entry &entry = m_entries[static_cast<std::size_t>(number)];
  if (!entry.loaded) {
    return nullptr;
  }
  const uint16_t wanted = flags & (FLIP_X | FLIP_Y);
  if (entry.orientation != wanted) {
    entry.orientation = wanted;
    refreshBox(static_cast<std::size_t>(number));
  }
  if ((flags & FLIP_Y) || !entry.masked) {
    return nullptr;
  }
  return (flags & FLIP_X) ? mirrored(number) : &entry.picture;
}

void ImageBank::forgetMirror(int number) {
  if (static_cast<std::size_t>(number) >= m_mirrorSlots.size()) {
    return;
  }
  const int16_t slot = m_mirrorSlots[static_cast<std::size_t>(number)];
  if (slot != NO_MIRROR) {
    dropMirror(static_cast<std::size_t>(slot));
  }
}

void ImageBank::dropMirror(std::size_t slot) {
  Mirror &mirror = m_mirrors[slot];
  m_mirrorBytes -= mirror.picture.pixels.size();
  retire(std::move(mirror.picture.pixels));
  m_mirrorSlots[static_cast<std::size_t>(mirror.number)] = NO_MIRROR;
  m_mirrors.erase(m_mirrors.begin() + static_cast<std::ptrdiff_t>(slot));
  for (std::size_t later = slot; later < m_mirrors.size(); ++later) {
    m_mirrorSlots[static_cast<std::size_t>(m_mirrors[later].number)] =
        static_cast<int16_t>(later);
  }
}

void ImageBank::setBeforeRetire(std::function<void(const uint8_t *)> hook) {
  m_beforeRetire = std::move(hook);
}

void ImageBank::beginPass() { ++m_pass; }

void ImageBank::releaseRetired() { m_retired.clear(); }

void ImageBank::retire(std::vector<uint8_t> &&pixels) {
  if (m_beforeRetire && !pixels.empty()) {
    m_beforeRetire(pixels.data());
    m_retired.push_back(std::move(pixels));
  }
}

bool ImageBank::isMasked(int number) const {
  return !find(number) || m_entries[static_cast<std::size_t>(number)].masked;
}

amal::Object &BobLayer::object(int number) {
  return m_bobs.at(static_cast<std::size_t>(number)).object;
}

BobLayer::Bob &BobLayer::activate(int number) {
  Bob &bob = m_bobs.at(static_cast<std::size_t>(number));
  bob.active = true;
  m_active[static_cast<std::size_t>(number / MASK_BITS)] |=
      1u << (number % MASK_BITS);
  return bob;
}

void BobLayer::set(int number, int x, int y, int image) {
  Bob &bob = activate(number);
  bob.object.x = static_cast<int16_t>(x);
  bob.object.y = static_cast<int16_t>(y);
  bob.object.image = static_cast<int16_t>(image);
}

void BobLayer::setPosition(int number, int x, int y) {
  Bob &bob = activate(number);
  bob.object.x = static_cast<int16_t>(x);
  bob.object.y = static_cast<int16_t>(y);
}

void BobLayer::setX(int number, int x) {
  Bob &bob = activate(number);
  bob.object.x = static_cast<int16_t>(x);
}

void BobLayer::setImage(int number, int image) {
  Bob &bob = activate(number);
  bob.object.image = static_cast<int16_t>(image);
}

void BobLayer::off(int number) {
  m_bobs.at(static_cast<std::size_t>(number)).active = false;
  m_active[static_cast<std::size_t>(number / MASK_BITS)] &=
      ~(1u << (number % MASK_BITS));
}

void BobLayer::offAll() {
  for (int word = 0; word < MASK_WORDS; ++word) {
    int number = word * MASK_BITS;
    for (uint32_t bits = m_active[static_cast<std::size_t>(word)]; bits != 0;
         bits >>= 1, ++number) {
      if ((bits & 1u) != 0) {
        m_bobs[static_cast<std::size_t>(number)].active = false;
      }
    }
  }
  m_active.fill(0);
}

bool BobLayer::collide(int number, const ImageBank &images, int first,
                       int last) {
  m_hits.fill(0);
  const Bob &testedBob = m_bobs.at(static_cast<std::size_t>(number));
  if (!testedBob.active) {
    return false;
  }
  const int testedImage =
      static_cast<uint16_t>(testedBob.object.image) & ImageBank::NUMBER_MASK;
  const ImageBank::Box *testedBox = images.box(testedImage);
  if (!testedBox) {
    return false;
  }
  const int testedLeft = testedBob.object.x - testedBox->hotX;
  const int testedTop = testedBob.object.y - testedBox->hotY;
  const MaskBox testedArea{testedLeft, testedTop, testedLeft + testedBox->width,
                           testedTop + testedBox->height};
  const int begin = std::max(first, 0);
  const int end = std::min(last, BOBS - 1);
  const ImageBank::Box *boxes = images.boxes();
  const int boxCount = images.boxCount();
  Shape tested;
  bool any = false;
  for (int word = begin / MASK_BITS; begin <= end && word <= end / MASK_BITS;
       ++word) {
    const int base = word * MASK_BITS;
    uint32_t bits = m_active[static_cast<std::size_t>(word)] &
                    rangeBits(begin - base, end - base);
    if (number / MASK_BITS == word) {
      bits &= ~(1u << (number % MASK_BITS));
    }
    int other = base;
    while (bits != 0) {
      if ((bits & LOW_BYTE) == 0) {
        bits >>= BYTE_BITS;
        other += BYTE_BITS;
        continue;
      }
      const int candidate = other;
      const bool present = (bits & 1u) != 0;
      bits >>= 1;
      ++other;
      if (!present) {
        continue;
      }
      const amal::Object &object =
          m_bobs[static_cast<std::size_t>(candidate)].object;
      const int image =
          static_cast<uint16_t>(object.image) & ImageBank::NUMBER_MASK;
      if (image == 0 || image >= boxCount) {
        continue;
      }
      const ImageBank::Box &box = boxes[image];
      if (box.height == 0) {
        continue;
      }
      const int left = object.x - box.hotX;
      if (left >= testedArea.right || testedLeft >= left + box.width) {
        continue;
      }
      const int top = object.y - box.hotY;
      if (top >= testedArea.bottom || testedTop >= top + box.height) {
        continue;
      }
      if (!tested.mask.picture) {
        tested = shapeAt(images.mask(testedImage), testedLeft, testedTop);
      }
      if (overlaps(tested, testedArea, shapeAt(images.mask(image), left, top),
                   MaskBox{left, top, left + box.width, top + box.height})) {
        m_hits[static_cast<std::size_t>(word)] |= 1u << (candidate - base);
        any = true;
      }
    }
  }
  return any;
}

bool BobLayer::collided(int number) const {
  if (number < 0 || number >= BOBS) {
    throw std::out_of_range("Bobs: no such bob");
  }
  return (m_hits[static_cast<std::size_t>(number / MASK_BITS)] >>
              (number % MASK_BITS) &
          1u) != 0;
}

const std::vector<BobLayer::Placement> &
BobLayer::placements(const IndexedSurface &surface,
                     const ImageBank &images) const {
  if (m_ordered != m_active) {
    m_order.clear();
    for (int word = 0; word < MASK_WORDS; ++word) {
      int number = word * MASK_BITS;
      for (uint32_t bits = m_active[static_cast<std::size_t>(word)]; bits != 0;
           bits >>= 1, ++number) {
        if ((bits & 1u) != 0) {
          m_order.push_back(number);
        }
      }
    }
    m_ordered = m_active;
  }
  int *const order = m_order.data();
  int *const orderEnd = order + m_order.size();
  uint32_t *keys = m_keys.data();
  for (const int *number = order; number != orderEnd; ++number) {
    const amal::Object &object =
        m_bobs[static_cast<std::size_t>(*number)].object;
    *keys++ = static_cast<uint32_t>(static_cast<uint16_t>(object.y ^ SIGN_BIT))
                  << 16 |
              static_cast<uint16_t>(object.x ^ SIGN_BIT);
  }
  keys = m_keys.data();
  for (int *next = order + 1; next < orderEnd; ++next) {
    const int number = *next;
    uint32_t *const nextKey = keys + (next - order);
    const uint32_t key = *nextKey;
    int *to = next;
    uint32_t *toKey = nextKey;
    while (to != order &&
           (toKey[-1] > key || (toKey[-1] == key && to[-1] > number))) {
      *to = to[-1];
      *toKey = toKey[-1];
      --to;
      --toKey;
    }
    *to = number;
    *toKey = key;
  }

  m_placed.clear();
  for (const int *number = order; number != orderEnd; ++number) {
    const amal::Object &bob = m_bobs[static_cast<std::size_t>(*number)].object;
    const uint16_t image = static_cast<uint16_t>(bob.image);
    const Picture *picture = images.find(image & ImageBank::NUMBER_MASK);
    if (!picture) {
      continue;
    }
    const uint16_t flags = image & (ImageBank::FLIP_X | ImageBank::FLIP_Y);
    const int left = bob.x - hotX(*picture, flags);
    const int top = bob.y - hotY(*picture, flags);
    if (surface.intersects(left, top, picture->width, picture->height)) {
      m_placed.push_back(
          {picture, left, top, static_cast<int16_t>(*number), flags});
    }
  }
  return m_placed;
}

void BobLayer::draw(IndexedSurface &surface, ImageBank &images) const {
  images.beginPass();
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
    const Picture *picture = placed.picture;
    bool flipX = (placed.flags & ImageBank::FLIP_X) != 0;
    if (flipX) {
      if (const Picture *mirror = images.mirrored(index)) {
        picture = mirror;
        flipX = false;
      }
    }
    surface.draw(*picture, placed.left, placed.top, flipX,
                 placed.flags & ImageBank::FLIP_Y, !images.isMasked(index));
  }
}

std::size_t BobLayer::drawSaving(IndexedSurface &surface, ImageBank &images,
                                 std::vector<SavedArea> &saved) const {
  images.beginPass();
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
  }
  for (std::size_t index = 0; index < count; ++index) {
    SavedArea &area = saved[index];
    area.pixels.copy(surface, area.left, area.top,
                     area.left + area.pixels.width(),
                     area.top + area.pixels.height(), 0, 0);
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

bool BobLayer::sprites(const IndexedSurface &surface, ImageBank &images,
                       std::vector<Sprite> &out) const {
  out.clear();
  images.beginPass();
  const std::vector<Placement> &placedBobs = placements(surface, images);
  if (placedBobs.size() > systems::graphics::SPRITE_SLOTS) {
    return false;
  }
  for (const Placement &placed : placedBobs) {
    const int index =
        static_cast<uint16_t>(
            m_bobs[static_cast<std::size_t>(placed.number)].object.image) &
        ImageBank::NUMBER_MASK;
    const Picture *picture = images.spriteImage(index, placed.flags);
    if (!picture) {
      return false;
    }
    if (!systems::graphics::canBeSprite(picture->pixels, picture->width)) {
      return false;
    }
    out.push_back({picture->pixels.data(), static_cast<int16_t>(picture->width),
                   static_cast<int16_t>(picture->height), placed.left,
                   placed.top});
  }
  return true;
}

std::size_t BobLayer::bake(IndexedSurface &surface,
                           const std::vector<Sprite> &sprites,
                           std::vector<SavedArea> &saved) {
  std::size_t count = 0;
  for (const Sprite &sprite : sprites) {
    const int words = (sprite.width + WORD_PIXELS - 1) / WORD_PIXELS +
                      ((sprite.left & (WORD_PIXELS - 1)) != 0 ? 1 : 0);
    const int start = wordStart(sprite.left);
    const int x1 = std::max(start, 0);
    const int x2 = std::min(start + words * WORD_PIXELS, surface.width());
    const int y1 = std::max(sprite.top, 0);
    const int y2 = std::min(sprite.top + sprite.height, surface.height());
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
  }
  for (std::size_t index = 0; index < count; ++index) {
    SavedArea &area = saved[index];
    area.pixels.copy(surface, area.left, area.top,
                     area.left + area.pixels.width(),
                     area.top + area.pixels.height(), 0, 0);
  }
  stamp(surface, sprites);
  return count;
}

void BobLayer::stamp(IndexedSurface &surface,
                     const std::vector<Sprite> &sprites) {
  for (const Sprite &sprite : sprites) {
    surface.draw(sprite.pixels, sprite.width, sprite.height, sprite.left,
                 sprite.top, false);
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
