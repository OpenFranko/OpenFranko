#ifndef SYSTEMS_JAGUAR_FRAMEBUILDER_H_
#define SYSTEMS_JAGUAR_FRAMEBUILDER_H_

#include "../graphics/Display.h"
#include "ObjectList.h"
#include "Video.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {

inline constexpr uint32_t COPPER_END = 0xFFFF0000u;
inline constexpr int HIRES_THRESHOLD = 480;
inline constexpr int SOLID_PHRASES = 256;

struct Placement {
  int left = 0;
  int top = 0;
  int halfWidth = 1;
  int rowsPerLine = 1;
};

struct LayerArea {
  int firstRow = 0;
  int lastRow = 0;
  int firstColumn = 0;
  int lastColumn = 0;
};

struct Translation {
  const uint8_t *source = nullptr;
  uint8_t *target = nullptr;
  std::size_t bytes = 0;
  uint32_t keep = 0xFFFFFFFFu;
  uint32_t flip = 0;
};

class TranslationBuffers {
public:
  virtual ~TranslationBuffers() = default;
  virtual uint8_t *buffer(const uint8_t *source, std::size_t bytes) = 0;
};

struct FrameMemory {
  uint32_t liveAddress = 0;
  uint32_t solidPhrases = 0;
  TranslationBuffers *buffers = nullptr;
  uint8_t *linePhrases = nullptr;
  int lineCapacity = 0;
};

struct BuiltFrame {
  std::vector<uint64_t> phrases;
  std::array<uint16_t, 256> clut{};
  uint32_t clutVersion = 0;
  std::vector<uint32_t> copper;
  std::vector<Translation> translations;
  graphics::RowColors lineRows;
  const uint8_t *lineTarget = nullptr;
  int lineFirst = 0;
  int lineCount = 0;
  int lineShift = 0;
  uint16_t lineDefault = 0;
  uint16_t background = 0;
  uint32_t border = 0;
  Placement placement;
  std::vector<LayerArea> areas;
  std::vector<int> objects;
  std::vector<int> layerTranslations;
  int lineObject = -1;
  int lineLayer = -1;
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
                const FrameMemory &memory, const Overlay *overlays,
                std::size_t overlayCount, BuiltFrame &frame);
bool scrollFrame(const graphics::Display &display,
                 const graphics::Display &built, const Geometry &geometry,
                 const FrameMemory &memory, BuiltFrame &frame);
void translateOnCpu(const Translation &translation);

} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_FRAMEBUILDER_H_
