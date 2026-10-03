#ifndef ENGINE_STREET_CORE_BOBS_H_
#define ENGINE_STREET_CORE_BOBS_H_

#include "../../../systems/graphics/PixelOps.h"
#include "../../amal/Machine.h"
#include "IndexedSurface.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace core {

using Paste = std::function<void(int x, int y, int image)>;

using RowSpan = systems::graphics::pixels::Span;

using MaskBox = systems::graphics::pixels::Bounds;

class ImageBank {
public:
  static constexpr uint16_t FLIP_X = 0x8000;
  static constexpr uint16_t FLIP_Y = 0x4000;
  static constexpr uint16_t NUMBER_MASK = 0x3FFF;
  static constexpr int FIRST_IMAGE = 1;

  static constexpr int BAND_ROWS = 4;

  struct Mask {
    const Picture *picture = nullptr;
    const RowSpan *rows = nullptr;
    const RowSpan *bands = nullptr;
    MaskBox box;
    uint16_t orientation = 0;
  };

  struct Box {
    int16_t hotX = 0;
    int16_t hotY = 0;
    int16_t width = 0;
    int16_t height = 0;
  };

  void clear();
  void load(int base, std::vector<Picture> frames);
  void load(int number, Picture picture);
  const Picture *find(int number) const;
  Mask mask(int number) const;
  uint16_t orientation(int number) const;
  void orient(int number, uint16_t flags);
  void noMask(int number);
  bool isMasked(int number) const;
  const Picture *mirrored(int number);
  const Box *box(int number) const {
    if (number <= 0 || static_cast<std::size_t>(number) >= m_boxes.size()) {
      return nullptr;
    }
    const Box &found = m_boxes[static_cast<std::size_t>(number)];
    return found.height != 0 ? &found : nullptr;
  }
  const Box *boxes() const { return m_boxes.data(); }
  int boxCount() const { return static_cast<int>(m_boxes.size()); }

private:
  struct Entry {
    Picture picture;
    uint16_t orientation = 0;
    bool loaded = false;
    bool masked = true;
  };

  struct Outline {
    std::unique_ptr<RowSpan[]> spans;
    std::size_t capacity = 0;
    MaskBox box;
  };

  struct Mirror {
    int number = 0;
    uint32_t used = 0;
    Picture picture;
  };

  void grow(std::size_t end);
  void store(std::size_t number, Picture &&picture);
  void refreshBox(std::size_t number);
  void forgetMirror(int number);

  std::vector<Entry> m_entries;
  std::vector<Outline> m_outlines;
  std::vector<Box> m_boxes;
  std::vector<Mirror> m_mirrors;
  std::size_t m_mirrorBytes = 0;
  uint32_t m_mirrorUses = 0;
};

struct SavedArea {
  int left = 0;
  int top = 0;
  IndexedSurface pixels = IndexedSurface(0, 0);
};

class BobLayer {
public:
  static constexpr int BOBS = 64;
  static constexpr int MASK_BITS = 32;
  static constexpr int MASK_WORDS = BOBS / MASK_BITS;

  amal::Object &object(int number);
  void set(int number, int x, int y, int image);
  void setPosition(int number, int x, int y);
  void setX(int number, int x);
  void setImage(int number, int image);
  void off(int number);
  void offAll();

  bool isActive(int number) const {
    return m_bobs.at(static_cast<std::size_t>(number)).active;
  }
  int16_t x(int number) const {
    return m_bobs.at(static_cast<std::size_t>(number)).object.x;
  }
  int16_t y(int number) const {
    return m_bobs.at(static_cast<std::size_t>(number)).object.y;
  }
  int16_t image(int number) const {
    return m_bobs.at(static_cast<std::size_t>(number)).object.image;
  }
  uint32_t activeBits(int word) const {
    return m_active[static_cast<std::size_t>(word)];
  }

  bool collide(int number, const ImageBank &images, int first = 0,
               int last = BOBS - 1);
  bool collided(int number) const;

  void draw(IndexedSurface &surface, ImageBank &images) const;
  std::size_t drawSaving(IndexedSurface &surface, ImageBank &images,
                         std::vector<SavedArea> &saved) const;
  static void restore(IndexedSurface &surface,
                      const std::vector<SavedArea> &saved, std::size_t count);
  static bool paste(IndexedSurface &surface, ImageBank &images, int x, int y,
                    int image);

private:
  struct Placement {
    int number = 0;
    const Picture *picture = nullptr;
    uint16_t flags = 0;
    int left = 0;
    int top = 0;
  };

  const std::vector<Placement> &placements(const IndexedSurface &surface,
                                           const ImageBank &images) const;
  void drawPlaced(IndexedSurface &surface, ImageBank &images,
                  const std::vector<Placement> &placed) const;

  struct Bob {
    bool active = false;
    amal::Object object;
  };

  Bob &activate(int number);

  std::array<Bob, BOBS> m_bobs{};
  std::array<uint32_t, MASK_WORDS> m_active{};
  std::array<uint32_t, MASK_WORDS> m_hits{};
  mutable std::vector<int> m_order;
  mutable std::vector<Placement> m_placed;
};

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CORE_BOBS_H_
