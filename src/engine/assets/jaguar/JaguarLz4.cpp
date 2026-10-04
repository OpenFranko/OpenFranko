#include "../Lz4.h"

#include "../../../systems/jaguar/Blitter.h"

namespace openfranko::src::engine::assets {

void unpackLz4(const uint8_t *source, std::size_t sourceSize, uint8_t *target,
               std::size_t targetSize) {
  if (!systems::jaguar::blitter::unpack(source, sourceSize, target,
                                        targetSize)) {
    decompressLz4(source, sourceSize, target, targetSize);
  }
}

void unpackLz4Part(const uint8_t *&source, const uint8_t *sourceEnd,
                   const uint8_t *start, uint8_t *&target,
                   const uint8_t *targetEnd, const uint8_t *limit) {
  const uint8_t *in = source;
  uint8_t *out = target;
  if (systems::jaguar::blitter::unpackPart(in, sourceEnd, out, targetEnd,
                                           limit) &&
      in <= sourceEnd && out <= targetEnd &&
      (in == sourceEnd || (limit && out >= limit))) {
    source = in;
    target = out;
    return;
  }
  decompressLz4Part(source, sourceEnd, start, target, targetEnd, limit);
}

} // namespace openfranko::src::engine::assets
