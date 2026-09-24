#ifndef ENGINE_STREET_BOBS_H_
#define ENGINE_STREET_BOBS_H_

#include "../amal/Machine.h"
#include "IndexedSurface.h"

#include <array>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

class ImageBank {
public:
  static constexpr uint16_t FLIP_X = 0x8000;
  static constexpr uint16_t FLIP_Y = 0x4000;
  static constexpr uint16_t NUMBER_MASK = 0x3FFF;

  void clear();
  void load(int base, const std::vector<Picture> &frames);
  const Picture *find(int number) const;
  uint16_t orientation(int number) const;
  void orient(int number, uint16_t flags);
  void noMask(int number);
  bool isMasked(int number) const;

private:
  struct Entry {
    Picture picture;
    uint16_t orientation = 0;
    bool loaded = false;
    bool masked = true;
  };

  std::vector<Entry> m_entries;
};

struct SavedArea {
  int left = 0;
  int top = 0;
  IndexedSurface pixels = IndexedSurface(0, 0);
};

class BobLayer {
public:
  static constexpr int COUNT = 64;

  amal::Object &object(int number);
  void set(int number, int x, int y, int image);
  void setPosition(int number, int x, int y);
  void setX(int number, int x);
  void setImage(int number, int image);
  void off(int number);
  void offAll();

  bool isActive(int number) const;
  int16_t x(int number) const;
  int16_t y(int number) const;
  int16_t image(int number) const;

  bool collide(int number, const ImageBank &images, int first = 0,
               int last = COUNT - 1);
  bool collided(int number) const;

  void draw(IndexedSurface &surface, ImageBank &images) const;
  std::vector<SavedArea> drawSaving(IndexedSurface &surface,
                                    ImageBank &images) const;
  static void restore(IndexedSurface &surface,
                      const std::vector<SavedArea> &saved);
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

  std::vector<Placement> placements(const IndexedSurface &surface,
                                    const ImageBank &images) const;

  struct Bob {
    bool active = false;
    amal::Object object;
  };

  std::array<Bob, COUNT> m_bobs{};
  std::array<bool, COUNT> m_collisions{};
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_BOBS_H_
