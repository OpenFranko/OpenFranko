#include "GameFiles.h"

#include "../../systems/jaguar/Hardware.h"
#include "../../systems/jaguar/Runtime.h"
#include "PackedFiles.h"

namespace openfranko::src::engine::assets {
namespace {

constexpr uint32_t CARTRIDGE_END = 0xE00000;

} // namespace

std::unique_ptr<Files> openGameFiles() {
  const uint32_t start = systems::jaguar::runtime::romEnd();
  return std::make_unique<PackedFiles>(reinterpret_cast<const uint8_t *>(start),
                                       CARTRIDGE_END - start);
}

} // namespace openfranko::src::engine::assets
