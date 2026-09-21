#include "VideoSystem.h"
#include <fstream>
#include <stdexcept>

namespace openfranko::src::systems {
namespace {

void throwError(const std::string &cause) {
  throw std::runtime_error("Video system error: " + cause);
}

constexpr auto WINDOW_NAME = "OpenFranko";
constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;

uint16_t readUint16LittleEndian(const char *bytes) {
  return static_cast<uint16_t>(
      static_cast<uint8_t>(bytes[0]) |
      (static_cast<uint16_t>(static_cast<uint8_t>(bytes[1])) << 8));
}

std::pair<int, int> parseImageHotspot(const std::string &path) {
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
  SDL_Texture *target =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, width, height);

  screens[screenId] = {screenId, width, height, target};
}

void VideoSystem::switchScreen(int screenId) {
  if (screens.find(screenId) != screens.end()) {
    currentScreenId = screenId;
    SDL_SetRenderTarget(renderer, screens[currentScreenId].targetTexture);
  }
}

void VideoSystem::sync() {
  SDL_SetRenderTarget(renderer, nullptr);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  auto &activeScr = screens[currentScreenId];

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

void VideoSystem::loadImage(const std::string &name, const std::string &path) {
  clearImage(name);
  imageStates.emplace(name, loadImageFile(path));
}

void VideoSystem::clearImage(const std::string &name) {
  auto it = imageStates.find(name);
  if (it == imageStates.end()) {
    return;
  }

  if (it->second.texture) {
    SDL_DestroyTexture(it->second.texture);
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

VideoSystem::Image VideoSystem::loadImageFile(const std::string &path) {
  SDL_Surface *tempSurface = IMG_Load(path.c_str());
  if (!tempSurface) {
    throwError("Failed to load image: " + path);
  }

  uint32_t colorKey = SDL_MapRGB(tempSurface->format, 85, 85, 85);
  SDL_SetColorKey(tempSurface, SDL_TRUE, colorKey);

  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, tempSurface);
  int width = tempSurface->w;
  int height = tempSurface->h;

  SDL_FreeSurface(tempSurface);

  auto hotspot = parseImageHotspot(path);

  return Image{tex, width, height, hotspot.first, hotspot.second};
}

} // namespace openfranko::src::systems