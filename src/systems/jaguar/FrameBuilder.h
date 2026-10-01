#ifndef SYSTEMS_JAGUAR_FRAMEBUILDER_H_
#define SYSTEMS_JAGUAR_FRAMEBUILDER_H_

#include "../graphics/Display.h"
#include "ObjectList.h"
#include "Video.h"

#include <array>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {

inline constexpr uint32_t COPPER_END = 0xFFFF0000u;
inline constexpr int HIRES_THRESHOLD = 480;

struct Placement {
  int left = 0;
  int top = 0;
  int halfWidth = 1;
};

struct LayerArea {
  int firstRow = 0;
  int lastRow = 0;
  int firstColumn = 0;
  int lastColumn = 0;
};

struct BuiltFrame {
  std::vector<uint64_t> phrases;
  std::array<uint16_t, 256> clut{};
  std::vector<uint32_t> copper;
  uint16_t background = 0;
  uint32_t border = 0;
};

Placement placeDisplay(const graphics::Display &display,
                       const Geometry &geometry);
LayerArea visibleArea(const graphics::Display &display,
                      const graphics::Layer &layer, const Placement &placement,
                      const Geometry &geometry);
struct Overlay {
  uint32_t pixels = 0;
  int width = 0;
  int height = 0;
  int column = 0;
  int row = 0;
};

bool sameLayout(const graphics::Display &left, const graphics::Display &right);
void allowCopper(bool allowed);
void buildFrame(const graphics::Display &display, const Geometry &geometry,
                uint32_t liveAddress, uint32_t solidPhrase,
                const Overlay *overlays, std::size_t overlayCount,
                BuiltFrame &frame);

} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_FRAMEBUILDER_H_
