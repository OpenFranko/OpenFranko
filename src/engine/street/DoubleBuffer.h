#ifndef ENGINE_STREET_DOUBLEBUFFER_H_
#define ENGINE_STREET_DOUBLEBUFFER_H_

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

class DoubleBuffer {
public:
  using Op = std::function<void(IndexedSurface &)>;

  explicit DoubleBuffer(const IndexedSurface &screen);

  const IndexedSurface &shown() const;
  IndexedSurface &logic();
  const IndexedSurface &logic() const;
  bool isAutobacking() const;
  bool isDirty(const BobLayer &bobs) const;

  void vbl();
  void test(const BobLayer &bobs, ImageBank &images);
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

  using Snapshot = std::array<BobState, BobLayer::COUNT>;

  static Snapshot snapshot(const BobLayer &bobs);
  void update(const BobLayer &bobs, ImageBank &images);

  std::array<Buffer, 2> m_buffers;
  int m_logic = 0;
  int m_shown = 1;
  bool m_vbl = false;
  bool m_updates = true;
  Snapshot m_drawn{};
  Op m_op;
  int m_phase = 0;
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_DOUBLEBUFFER_H_
