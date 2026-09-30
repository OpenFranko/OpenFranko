#include "Bobs.h"

#include <algorithm>
#include <stdexcept>

namespace openfranko::src::engine::street::core {
namespace {

struct Shape {
  ImageBank::Mask mask;
  int left = 0;
  int top = 0;
};

struct MaskRows {
  const uint8_t *pixels = nullptr;
  const RowSpan *spans = nullptr;
  int row = 0;
  int rowStep = 1;
  int offset = 0;
  int offsetStep = 0;
  int left = 0;
  int right = 0;
  bool flipX = false;
};

struct SolidRun {
  int from = 0;
  int to = -1;
  int pixel = 0;
  int step = 1;
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

std::vector<RowSpan> rowSpans(const Picture &picture) {
  std::vector<RowSpan> rows(static_cast<std::size_t>(picture.height));
  const uint8_t *row = picture.pixels.data();
  for (RowSpan &span : rows) {
    int first = 0;
    while (first < picture.width && row[first] == 0) {
      ++first;
    }
    if (first < picture.width) {
      int last = picture.width - 1;
      while (row[last] == 0) {
        --last;
      }
      span.first = first;
      span.last = last;
    }
    row += picture.width;
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

MaskRows maskRows(const Shape &shape, int top) {
  const Picture &picture = *shape.mask.picture;
  const bool flipY = (shape.mask.orientation & ImageBank::FLIP_Y) != 0;
  MaskRows rows;
  rows.pixels = picture.pixels.data();
  rows.spans = shape.mask.rows;
  rows.row = flipY ? picture.height - 1 - (top - shape.top) : top - shape.top;
  rows.rowStep = flipY ? -1 : 1;
  rows.offset = rows.row * picture.width;
  rows.offsetStep = flipY ? -picture.width : picture.width;
  rows.left = shape.left;
  rows.right = shape.left + picture.width - 1;
  rows.flipX = (shape.mask.orientation & ImageBank::FLIP_X) != 0;
  return rows;
}

SolidRun solidRun(const MaskRows &rows) {
  const RowSpan &span = rows.spans[rows.row];
  SolidRun run;
  if (rows.flipX) {
    run.from = rows.right - span.last;
    run.to = rows.right - span.first;
    run.pixel = rows.offset + rows.right;
    run.step = -1;
  } else {
    run.from = rows.left + span.first;
    run.to = rows.left + span.last;
    run.pixel = rows.offset - rows.left;
  }
  return run;
}

void nextRow(MaskRows &rows) {
  rows.row += rows.rowStep;
  rows.offset += rows.offsetStep;
}

bool overlaps(const Shape &a, const Shape &b) {
  const MaskBox firstBox = screenBox(a);
  const MaskBox secondBox = screenBox(b);
  const int left = std::max(firstBox.left, secondBox.left);
  const int right = std::min(firstBox.right, secondBox.right);
  const int top = std::max(firstBox.top, secondBox.top);
  const int bottom = std::min(firstBox.bottom, secondBox.bottom);
  if (left >= right || top >= bottom) {
    return false;
  }
  MaskRows first = maskRows(a, top);
  MaskRows second = maskRows(b, top);
  for (int y = top; y < bottom; ++y) {
    const SolidRun firstRun = solidRun(first);
    const SolidRun secondRun = solidRun(second);
    const int from = std::max(left, std::max(firstRun.from, secondRun.from));
    const int to = std::min(right - 1, std::min(firstRun.to, secondRun.to));
    int firstPixel =
        firstRun.step < 0 ? firstRun.pixel - from : firstRun.pixel + from;
    int secondPixel =
        secondRun.step < 0 ? secondRun.pixel - from : secondRun.pixel + from;
    for (int x = from; x <= to; ++x) {
      if (first.pixels[firstPixel] != 0 && second.pixels[secondPixel] != 0) {
        return true;
      }
      firstPixel += firstRun.step;
      secondPixel += secondRun.step;
    }
    nextRow(first);
    nextRow(second);
  }
  return false;
}

} // namespace

void ImageBank::clear() {
  m_entries.clear();
  m_outlines.clear();
}

void ImageBank::load(int base, const std::vector<Picture> &frames) {
  const std::size_t end = static_cast<std::size_t>(base) + frames.size();
  if (m_entries.size() < end) {
    m_entries.resize(end);
    m_outlines.resize(end);
  }
  for (std::size_t i = 0; i < frames.size(); ++i) {
    Entry &entry = m_entries[static_cast<std::size_t>(base) + i];
    Outline &outline = m_outlines[static_cast<std::size_t>(base) + i];
    entry.picture = frames[i];
    outline.rows = rowSpans(frames[i]);
    outline.box = maskBox(outline.rows);
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

std::vector<BobLayer::Placement>
BobLayer::placements(const IndexedSurface &surface,
                     const ImageBank &images) const {
  std::vector<int> order;
  for (int number = 0; number < BOBS; ++number) {
    if (m_bobs[static_cast<std::size_t>(number)].active) {
      order.push_back(number);
    }
  }
  std::stable_sort(order.begin(), order.end(), [this](int a, int b) {
    const amal::Object &first = m_bobs[static_cast<std::size_t>(a)].object;
    const amal::Object &second = m_bobs[static_cast<std::size_t>(b)].object;
    return first.y != second.y ? first.y < second.y : first.x < second.x;
  });

  std::vector<Placement> placed;
  for (int number : order) {
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
      placed.push_back({number, picture, flags, left, top});
    }
  }
  return placed;
}

void BobLayer::draw(IndexedSurface &surface, ImageBank &images) const {
  for (const Placement &placed : placements(surface, images)) {
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

std::vector<SavedArea> BobLayer::drawSaving(IndexedSurface &surface,
                                            ImageBank &images) const {
  std::vector<SavedArea> saved;
  for (const Placement &placed : placements(surface, images)) {
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
    SavedArea area{x1, y1, IndexedSurface(x2 - x1, y2 - y1)};
    area.pixels.copy(surface, x1, y1, x2, y2, 0, 0);
    saved.push_back(std::move(area));
  }
  draw(surface, images);
  return saved;
}

void BobLayer::restore(IndexedSurface &surface,
                       const std::vector<SavedArea> &saved) {
  for (const SavedArea &area : saved) {
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
