#ifndef ENGINE_STREET_CORE_DOUBLEBUFFER_H_
#define ENGINE_STREET_CORE_DOUBLEBUFFER_H_

#include "Bobs.h"
#include "IndexedSurface.h"

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace core {

class DoubleBuffer {
public:
  using Op = std::function<void(IndexedSurface &)>;

  struct View {
    const IndexedSurface &pixels;
    const std::vector<Sprite> &sprites;
    uint32_t version;
  };

  explicit DoubleBuffer(const IndexedSurface &screen);
  DoubleBuffer(int width, int height);
  DoubleBuffer(const DoubleBuffer &other);
  DoubleBuffer(DoubleBuffer &&other);
  DoubleBuffer &operator=(const DoubleBuffer &other);
  DoubleBuffer &operator=(DoubleBuffer &&other);
  ~DoubleBuffer() = default;

  const IndexedSurface &shown() const;
  const IndexedSurface &upcoming() const;
  View shownView() const;
  View upcomingView() const;
  IndexedSurface &logic();
  const IndexedSurface &logic() const;
  void setSprites(bool on);
  void bake();
  void bakeUsing(const uint8_t *pixels);
  bool isAutobacking() const;
  bool isDirty(const BobLayer &bobs) const;

  void vbl();
  bool test(const BobLayer &bobs, ImageBank &images);
  void setUpdates(bool on);

  void clearBobs();
  void drawBobs(const BobLayer &bobs, ImageBank &images);
  void swap();

  void autoback(Op op);
  void autobackStep(const BobLayer &bobs, ImageBank &images);

private:
  struct Buffer {
    IndexedSurface pixels;
    std::vector<SavedArea> saved;
    std::size_t savedCount = 0;
    std::vector<Sprite> sprites;
    uint32_t version = 0;
  };

  struct BobState {
    bool active = false;
    int16_t x = 0;
    int16_t y = 0;
    int16_t image = 0;

    bool operator==(const BobState &other) const {
      return active == other.active && x == other.x && y == other.y &&
             image == other.image;
    }
  };

  struct Snapshot {
    std::array<BobState, BobLayer::BOBS> bobs{};
    std::array<uint32_t, BobLayer::MASK_WORDS> active{};
  };

  static void snapshot(const BobLayer &bobs, Snapshot &state);
  static void bakeBuffer(Buffer &buffer);
  void adopt();
  void update(const BobLayer &bobs, ImageBank &images);

  mutable std::array<Buffer, 2> m_buffers;
  int m_logic = 0;
  int m_shown = 1;
  bool m_vbl = false;
  bool m_updates = true;
  Snapshot m_drawn{};
  Op m_op;
  int m_phase = 0;
  bool m_sprites = false;
  std::vector<Sprite> m_recording;
};

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CORE_DOUBLEBUFFER_H_
