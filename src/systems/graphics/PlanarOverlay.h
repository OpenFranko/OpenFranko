#ifndef SYSTEMS_GRAPHICS_PLANAROVERLAY_H_
#define SYSTEMS_GRAPHICS_PLANAROVERLAY_H_

#include "graphics/IndexedRasterizer.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {

class PlanarOverlay {
public:
  static constexpr int PLANES = 4;
  static constexpr int WORD_BYTES = 4;
  static constexpr int ENTRY_BYTES = 2 * WORD_BYTES;

  struct Words {
    int first = 0;
    int count = 0;
    const uint8_t *entries = nullptr;
  };

  bool isBuiltFrom(const Overlay &overlay) const;
  void build(const Overlay &overlay);
  void composite(int plane, int row, uint8_t *line) const;
  const Words *words(int plane, int row) const;
  int top() const;
  int lastWord() const;

private:
  bool m_built = false;
  uint32_t m_revision = 0;
  int m_left = 0;
  int m_top = 0;
  int m_width = 0;
  int m_height = 0;
  int m_lastWord = 0;
  std::vector<Words> m_words;
  std::vector<uint8_t> m_entries;
};

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_PLANAROVERLAY_H_
