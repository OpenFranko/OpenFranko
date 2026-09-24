#include "VideoSystem.h"
#include "Bitmap.h"
#include <algorithm>
#include <stdexcept>

namespace openfranko::src::systems {
namespace {

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Video system error: " + cause);
}

uint8_t nibbleToChannel(int nibble) {
  return static_cast<uint8_t>((nibble & 0xF) * 17);
}

int channelToNibble(uint8_t channel) { return (channel + 8) / 17; }

constexpr auto WINDOW_NAME = "OpenFranko";
constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;

} // namespace

VideoSystem::VideoSystem()
    : window(nullptr), renderer(nullptr), currentScreenId(0) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
    throwError("SDL initialization failed");
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    throwError("SDL_image initialization failed");
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

  window = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (!window) {
    throwError("Window creation failed");
  }

  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    throwError("Renderer creation failed");
  }
}

VideoSystem::~VideoSystem() {
  for (auto &pair : imageStates) {
    if (pair.second.texture) {
      SDL_DestroyTexture(pair.second.texture);
    }
    if (pair.second.indexedSurface) {
      SDL_FreeSurface(pair.second.indexedSurface);
    }
  }

  for (auto &pair : screens) {
    if (pair.second.targetTexture) {
      SDL_DestroyTexture(pair.second.targetTexture);
    }
  }

  imageStates.clear();
  screens.clear();

  if (renderer) {
    SDL_DestroyRenderer(renderer);
  }
  if (window) {
    SDL_DestroyWindow(window);
  }

  IMG_Quit();
}

void VideoSystem::createScreen(int screenId, int width, int height) {
  auto it = screens.find(screenId);
  if (it != screens.end()) {
    if (it->second.targetTexture) {
      SDL_DestroyTexture(it->second.targetTexture);
    }
  }

  SDL_Texture *target =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, width, height);

  if (!target) {
    throwError("Failed to create screen texture: " +
               std::string(SDL_GetError()));
  }

  SDL_SetTextureBlendMode(target, SDL_BLENDMODE_BLEND);

  screens[screenId] = {screenId, width, height, target};

  if (currentScreenId == screenId) {
    SDL_SetRenderTarget(renderer, target);
  }
}

void VideoSystem::switchScreen(int screenId) {
  if (screens.find(screenId) != screens.end()) {
    currentScreenId = screenId;
    SDL_SetRenderTarget(renderer, screens[currentScreenId].targetTexture);
  }
}

void VideoSystem::destroyScreen(int screenId) {
  auto it = screens.find(screenId);

  if (it == screens.end()) {
    return;
  }

  if (currentScreenId == screenId) {
    SDL_SetRenderTarget(renderer, nullptr);
  }

  if (it->second.targetTexture) {
    SDL_DestroyTexture(it->second.targetTexture);
  }

  screens.erase(it);
}

void VideoSystem::fillScreen(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  if (a < 255) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  } else {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  }

  SDL_SetRenderDrawColor(renderer, r, g, b, a);

  SDL_RenderFillRect(renderer, nullptr);
}

void VideoSystem::sync() {
  SDL_SetRenderTarget(renderer, nullptr);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  auto it = screens.find(currentScreenId);

  if (it == screens.end() || !it->second.targetTexture ||
      it->second.height == 0) {
    SDL_RenderPresent(renderer);
    return;
  }

  const auto &activeScr = it->second;

  int windowWidth, windowHeight;
  SDL_GetWindowSize(window, &windowWidth, &windowHeight);

  float targetAspect = static_cast<float>(activeScr.width) / activeScr.height;
  float windowAspect = static_cast<float>(windowWidth) / windowHeight;

  SDL_Rect dstRect;

  if (windowAspect > targetAspect) {
    dstRect.h = windowHeight;
    dstRect.w = static_cast<int>(windowHeight * targetAspect);
    dstRect.x = (windowWidth - dstRect.w) / 2;
    dstRect.y = 0;
  } else {
    dstRect.w = windowWidth;
    dstRect.h = static_cast<int>(windowWidth / targetAspect);
    dstRect.x = 0;
    dstRect.y = (windowHeight - dstRect.h) / 2;
  }

  SDL_RenderCopy(renderer, activeScr.targetTexture, nullptr, &dstRect);
  SDL_RenderPresent(renderer);
  SDL_SetRenderTarget(renderer, activeScr.targetTexture);
}

void VideoSystem::loadImage(const std::string &name, const std::string &path,
                            bool applyColorKey) {
  clearImage(name);
  imageStates.emplace(name, loadImageFile(path, applyColorKey));
}

void VideoSystem::clearImage(const std::string &name) {
  auto it = imageStates.find(name);
  if (it == imageStates.end()) {
    return;
  }

  if (it->second.texture) {
    SDL_DestroyTexture(it->second.texture);
  }
  if (it->second.indexedSurface) {
    SDL_FreeSurface(it->second.indexedSurface);
  }
  imageStates.erase(it);
}

void VideoSystem::drawImage(const std::string &name, int x, int y,
                            SDL_RendererFlip flip) {
  auto it = imageStates.find(name);
  if (it == imageStates.end()) {
    return;
  }

  const auto &img = it->second;

  SDL_Rect srcRect = {0, 0, img.width, img.height};

  int currentHotX = img.hotspotX;
  int currentHotY = img.hotspotY;

  if (flip & SDL_FLIP_HORIZONTAL) {
    currentHotX = img.width - img.hotspotX;
  }

  SDL_Rect dstRect = {x - currentHotX, y - currentHotY, img.width, img.height};
  SDL_Point pivot = {currentHotX, currentHotY};

  SDL_RenderCopyEx(renderer, img.texture, &srcRect, &dstRect, 0.0, &pivot,
                   flip);
}

VideoSystem::Image VideoSystem::loadImageFile(const std::string &path,
                                              bool applyColorKey) {
  SDL_Surface *tempSurface = IMG_Load(path.c_str());
  if (!tempSurface) {
    throwError("Failed to load image: " + path);
  }

  if (applyColorKey) {
    uint32_t colorKey = SDL_MapRGB(tempSurface->format, 85, 85, 85);
    SDL_SetColorKey(tempSurface, SDL_TRUE, colorKey);
  }

  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, tempSurface);
  int width = tempSurface->w;
  int height = tempSurface->h;

  SDL_FreeSurface(tempSurface);

  auto hotspot = readBitmapHotspot(path);

  return Image{tex, width, height, hotspot.first, hotspot.second};
}

void VideoSystem::loadIndexedImage(const std::string &name,
                                   const std::string &path) {
  addIndexedImage(name, path, false);
}

void VideoSystem::loadMaskedImage(const std::string &name,
                                  const std::string &path) {
  addIndexedImage(name, path, true);
}

std::vector<uint16_t>
VideoSystem::getImagePalette(const std::string &name) const {
  const SDL_Palette *palette =
      findIndexedImage(name).indexedSurface->format->palette;

  std::vector<uint16_t> colors;
  colors.reserve(palette->ncolors);
  for (int i = 0; i < palette->ncolors; ++i) {
    const SDL_Color &color = palette->colors[i];
    colors.push_back(static_cast<uint16_t>(channelToNibble(color.r) << 8 |
                                           channelToNibble(color.g) << 4 |
                                           channelToNibble(color.b)));
  }
  return colors;
}

void VideoSystem::setImagePalette(const std::string &name,
                                  const std::vector<uint16_t> &palette) {
  Image &image = findIndexedImage(name);
  SDL_Palette *surfacePalette = image.indexedSurface->format->palette;

  const int count =
      std::min(static_cast<int>(palette.size()), surfacePalette->ncolors);
  std::vector<SDL_Color> colors(count);
  for (int i = 0; i < count; ++i) {
    colors[i] = {nibbleToChannel(palette[i] >> 8),
                 nibbleToChannel(palette[i] >> 4), nibbleToChannel(palette[i]),
                 255};
  }
  SDL_SetPaletteColors(surfacePalette, colors.data(), 0, count);

  refreshTexture(image, name);
}

void VideoSystem::updateFrameImage(const std::string &name, int width,
                                   int height,
                                   const std::vector<uint32_t> &argb) {
  auto it = imageStates.find(name);
  if (it == imageStates.end() || it->second.width != width ||
      it->second.height != height || it->second.indexedSurface) {
    clearImage(name);
    SDL_Texture *texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                          SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture) {
      throwError("Failed to create frame texture for: " + name);
    }
    it = imageStates.emplace(name, Image{texture, width, height, 0, 0}).first;
  }
  if (argb.size() != static_cast<std::size_t>(width * height)) {
    throwError("Frame of the wrong size for: " + name);
  }
  SDL_UpdateTexture(it->second.texture, nullptr, argb.data(),
                    width * static_cast<int>(sizeof(uint32_t)));
}

void VideoSystem::xorImageRect(const std::string &name, int x, int y, int width,
                               int height, uint8_t mask) {
  Image &image = findIndexedImage(name);
  SDL_Surface *surface = image.indexedSurface;

  const int left = std::max(x, 0);
  const int top = std::max(y, 0);
  const int right = std::min(x + width, surface->w);
  const int bottom = std::min(y + height, surface->h);

  SDL_LockSurface(surface);
  for (int row = top; row < bottom; ++row) {
    uint8_t *pixels =
        static_cast<uint8_t *>(surface->pixels) + row * surface->pitch;
    for (int column = left; column < right; ++column) {
      pixels[column] ^= mask;
    }
  }
  SDL_UnlockSurface(surface);

  refreshTexture(image, name);
}

const VideoSystem::Image &
VideoSystem::findIndexedImage(const std::string &name) const {
  auto it = imageStates.find(name);
  if (it == imageStates.end() || !it->second.indexedSurface) {
    throwError("No indexed image named: " + name);
  }
  return it->second;
}

VideoSystem::Image &VideoSystem::findIndexedImage(const std::string &name) {
  auto it = imageStates.find(name);
  if (it == imageStates.end() || !it->second.indexedSurface) {
    throwError("No indexed image named: " + name);
  }
  return it->second;
}

void VideoSystem::refreshTexture(Image &image, const std::string &name) {
  SDL_Texture *texture =
      SDL_CreateTextureFromSurface(renderer, image.indexedSurface);
  if (!texture) {
    throwError("Failed to update texture for: " + name);
  }
  SDL_DestroyTexture(image.texture);
  image.texture = texture;
}

void VideoSystem::addIndexedImage(const std::string &name,
                                  const std::string &path,
                                  bool colorZeroTransparent) {
  clearImage(name);

  SDL_Surface *surface = SDL_LoadBMP(path.c_str());
  if (!surface) {
    throwError("Failed to load image: " + path);
  }
  if (surface->format->BitsPerPixel != 8 || !surface->format->palette) {
    SDL_FreeSurface(surface);
    throwError("Not an 8-bit indexed image: " + path);
  }
  if (colorZeroTransparent) {
    SDL_SetColorKey(surface, SDL_TRUE, 0);
  }

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture) {
    SDL_FreeSurface(surface);
    throwError("Failed to create texture for: " + path);
  }

  auto hotspot = readBitmapHotspot(path);
  imageStates.emplace(name, Image{texture, surface->w, surface->h,
                                  hotspot.first, hotspot.second, surface});
}

} // namespace openfranko::src::systems