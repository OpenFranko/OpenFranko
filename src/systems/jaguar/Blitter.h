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

void wait();
void useQueue(uint32_t control);
void stopQueue();
bool isQueued();
bool unpack(const uint8_t *source, std::size_t sourceSize, uint8_t *target,
            std::size_t targetSize);
bool outline(const uint8_t *pixels, int width, int height, void *rows,
             void *bands, int32_t *box);
bool flipSigns(const uint8_t *source, int8_t *target, std::size_t count);
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
