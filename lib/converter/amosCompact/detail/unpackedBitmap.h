#ifndef UNPACKEDBITMAP_H_
#define UNPACKEDBITMAP_H_

#include <cstdint>
#include <cstdlib>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace amosCompact {
namespace detail {

struct UnpackedBitmap {
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t numberOfBitplanes = 0;
  uint16_t palette[32] = {};
  uint8_t *bitplaneData[6] = {};
  size_t bytesPerPlane = 0;
  uint8_t *chunkyPixels = nullptr;

  ~UnpackedBitmap() {
    for (int i = 0; i < 6; i++) {
      free(bitplaneData[i]);
    }
    free(chunkyPixels);
  }

  UnpackedBitmap() = default;
  UnpackedBitmap(const UnpackedBitmap &) = delete;
  UnpackedBitmap &operator=(const UnpackedBitmap &) = delete;
  UnpackedBitmap(UnpackedBitmap &&other) noexcept
      : width(other.width), height(other.height),
        numberOfBitplanes(other.numberOfBitplanes),
        bytesPerPlane(other.bytesPerPlane), chunkyPixels(other.chunkyPixels) {
    for (int i = 0; i < 32; i++) {
      palette[i] = other.palette[i];
    }
    for (int i = 0; i < 6; i++) {
      bitplaneData[i] = other.bitplaneData[i];
      other.bitplaneData[i] = nullptr;
    }
    other.chunkyPixels = nullptr;
  }
  UnpackedBitmap &operator=(UnpackedBitmap &&other) noexcept {
    if (this != &other) {
      for (int i = 0; i < 6; i++) {
        free(bitplaneData[i]);
      }
      free(chunkyPixels);
      width = other.width;
      height = other.height;
      numberOfBitplanes = other.numberOfBitplanes;
      bytesPerPlane = other.bytesPerPlane;
      chunkyPixels = other.chunkyPixels;
      for (int i = 0; i < 32; i++) {
        palette[i] = other.palette[i];
      }
      for (int i = 0; i < 6; i++) {
        bitplaneData[i] = other.bitplaneData[i];
        other.bitplaneData[i] = nullptr;
      }
      other.chunkyPixels = nullptr;
    }
    return *this;
  }
};

} // namespace detail
} // namespace amosCompact
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // UNPACKEDBITMAP_H_