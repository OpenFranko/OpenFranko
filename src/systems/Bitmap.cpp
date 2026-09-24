#include "Bitmap.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace openfranko::src::systems {
namespace {

uint16_t readUint16LittleEndian(const char *bytes) {
  return static_cast<uint16_t>(
      static_cast<uint8_t>(bytes[0]) |
      (static_cast<uint16_t>(static_cast<uint8_t>(bytes[1])) << 8));
}

} // namespace

std::pair<int, int> readBitmapHotspot(const std::string &path) {
  auto hotspotValues = std::make_pair(0, 0);

  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return hotspotValues;
  }

  char header[10]{};
  file.read(header, sizeof(header));
  if (file.gcount() != sizeof(header) || header[0] != 'B' || header[1] != 'M') {
    return hotspotValues;
  }

  hotspotValues.first = readUint16LittleEndian(header + 6);
  hotspotValues.second = readUint16LittleEndian(header + 8);

  return hotspotValues;
}

IndexedBitmap loadIndexedBitmap(const std::string &path) {
  SDL_Surface *surface = SDL_LoadBMP(path.c_str());
  if (!surface) {
    throw std::runtime_error("Failed to load bitmap: " + path);
  }
  if (surface->format->BitsPerPixel != 8) {
    SDL_FreeSurface(surface);
    throw std::runtime_error("Not an 8-bit indexed bitmap: " + path);
  }

  IndexedBitmap bitmap;
  bitmap.width = surface->w;
  bitmap.height = surface->h;
  bitmap.pixels.resize(static_cast<std::size_t>(surface->w * surface->h));
  SDL_LockSurface(surface);
  for (int y = 0; y < surface->h; ++y) {
    const auto *row =
        static_cast<const uint8_t *>(surface->pixels) + y * surface->pitch;
    std::copy(row, row + surface->w,
              bitmap.pixels.begin() +
                  static_cast<std::ptrdiff_t>(y) * surface->w);
  }
  SDL_UnlockSurface(surface);
  SDL_FreeSurface(surface);

  const auto hotspot = readBitmapHotspot(path);
  bitmap.hotspotX = hotspot.first;
  bitmap.hotspotY = hotspot.second;
  return bitmap;
}

} // namespace openfranko::src::systems
