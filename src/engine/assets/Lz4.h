#ifndef ENGINE_ASSETS_LZ4_H_
#define ENGINE_ASSETS_LZ4_H_

#include <cstddef>
#include <cstdint>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

void decompressLz4(const uint8_t *source, std::size_t sourceSize,
                   uint8_t *target, std::size_t targetSize);
void decompressLz4Part(const uint8_t *&source, const uint8_t *sourceEnd,
                       const uint8_t *start, uint8_t *&target,
                       const uint8_t *targetEnd, const uint8_t *limit);
void unpackLz4(const uint8_t *source, std::size_t sourceSize, uint8_t *target,
               std::size_t targetSize);
void unpackLz4Part(const uint8_t *&source, const uint8_t *sourceEnd,
                   const uint8_t *start, uint8_t *&target,
                   const uint8_t *targetEnd, const uint8_t *limit);

class Lz4Steps {
public:
  Lz4Steps(const uint8_t *source, std::size_t sourceSize, uint8_t *target,
           std::size_t targetSize);

  bool step(std::size_t bytes);

private:
  const uint8_t *m_source;
  const uint8_t *m_sourceEnd;
  const uint8_t *m_start;
  uint8_t *m_target;
  const uint8_t *m_targetEnd;
};

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_LZ4_H_
