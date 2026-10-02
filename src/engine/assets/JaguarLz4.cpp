#include "Lz4.h"

#include "../../systems/jaguar/Blitter.h"

namespace openfranko::src::engine::assets {

void unpackLz4(const uint8_t *source, std::size_t sourceSize, uint8_t *target,
               std::size_t targetSize) {
  if (!systems::jaguar::blitter::unpack(source, sourceSize, target,
                                        targetSize)) {
    decompressLz4(source, sourceSize, target, targetSize);
  }
}

} // namespace openfranko::src::engine::assets
