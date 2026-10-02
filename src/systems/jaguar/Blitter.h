#ifndef SYSTEMS_JAGUAR_BLITTER_H_
#define SYSTEMS_JAGUAR_BLITTER_H_

#include <cstddef>
#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {
namespace blitter {

struct Area {
  uint8_t *pixels = nullptr;
  int pitch = 0;
};

struct Source {
  const uint8_t *pixels = nullptr;
  int pitch = 0;
};

enum class Mode : uint8_t { Copy, Masked, Mirrored, MirroredMasked };

void wait();
void useQueue(uint32_t control);
void stopQueue();
bool queue(Source source, Area target, int width, int height, Mode mode);
bool isQueued();
bool unpack(const uint8_t *source, std::size_t sourceSize, uint8_t *target,
            std::size_t targetSize);
bool unpackPart(const uint8_t *&source, const uint8_t *sourceEnd,
                uint8_t *&target, const uint8_t *targetEnd,
                const uint8_t *limit);
bool outline(const uint8_t *pixels, int width, int height, void *rows,
             void *bands, int32_t *box);
bool flipSigns(const uint8_t *source, int8_t *target, std::size_t count);
bool xorCopy(const uint8_t *source, uint8_t *target, std::size_t count,
             uint32_t mask);
void copy(Source source, Area target, int width, int height);
void copyMasked(Source source, Area target, int width, int height);
void copyMirrored(Source source, Area target, int width, int height,
                  bool masked);
void fill(Area target, int width, int height, uint8_t value);

} // namespace blitter
} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_BLITTER_H_
