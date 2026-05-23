#include "VideoSystem.h"
#include <fstream>
#include <stdexcept>

namespace openfranko::src::systems {
namespace {
void throwError(const std::string &cause) {
  throw std::runtime_error("Video system could not be initialised!: " + cause);
}

constexpr auto WINDOW_NAME = "OpenFranko";
constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;

uint16_t readUint16LittleEndian(const char *bytes) {
  return static_cast<uint16_t>(
      static_cast<uint8_t>(bytes[0]) |
      (static_cast<uint16_t>(static_cast<uint8_t>(bytes[1])) << 8));
}

std::pair<int, int> parseFrameHotspot(const std::string &path) {
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
    throwError("SDL");
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    throwError("IMG");
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

  window = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (!window) {
    throwError("Window");
  }

  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    throwError("Renderer");
  }
}

VideoSystem::~VideoSystem() {
  for (auto &state : animationStates) {
    for (auto &frame : state.second) {
      SDL_DestroyTexture(frame.texture);
    }
  }

  SDL_DestroyTexture(background);

  for (auto &pair : screens) {
    SDL_DestroyTexture(pair.second.targetTexture);
  }
  animationStates.clear();
  screens.clear();

  if (renderer)
    SDL_DestroyRenderer(renderer);
  if (window)
    SDL_DestroyWindow(window);
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

void VideoSystem::loadAnimation(const std::string &name,
                                const std::vector<std::string> &framePaths) {
  std::vector<AnimationFrame> frames;

  for (auto &path : framePaths) {
    const auto frame = loadFrame(path);
    frames.emplace_back(frame);
  }

  animationStates.emplace(name, frames);
}

void VideoSystem::clearAnimation(const std::string &name) {
  if (animationStates.find(name) == animationStates.end()) {
    return;
  }

  auto &frames = animationStates.at(name);

  for (auto &frame : frames) {
    SDL_DestroyTexture(frame.texture);
  }

  animationStates.erase(name);
}

void VideoSystem::loadBackground(const std::string &path) {
  SDL_Surface *tempSurface = IMG_Load(path.c_str());
  background = SDL_CreateTextureFromSurface(renderer, tempSurface);
  SDL_FreeSurface(tempSurface);
}

void VideoSystem::clearBackground() { SDL_DestroyTexture(background); }

void VideoSystem::drawAnimationFrame(const std::string &name, int x, int y,
                                     int frame, SDL_RendererFlip flip) {
  if (animationStates.find(name) == animationStates.end()) {
    return;
  }

  const auto frames = animationStates.at(name);

  if (frame >= frames.size()) {
    return;
  }

  const auto animFrame = frames.at(frame);

  SDL_Rect srcRect = {0, 0, animFrame.width, animFrame.height};

  int currentHotX = animFrame.hotspotX;
  int currentHotY = animFrame.hotspotY;

  if (flip & SDL_FLIP_HORIZONTAL) {
    currentHotX = animFrame.width - animFrame.hotspotX;
  }

  SDL_Rect dstRect = {x - currentHotX, y - currentHotY, animFrame.width,
                      animFrame.height};

  SDL_Point pivot = {currentHotX, currentHotY};

  SDL_RenderCopyEx(renderer, animFrame.texture, &srcRect, &dstRect, 0.0, &pivot,
                   flip);
}

size_t VideoSystem::getAnimationSize(const std::string &name) {
  if (animationStates.find(name) == animationStates.end()) {
    return 0;
  }

  const auto frames = animationStates.at(name);

  return frames.size();
}

void VideoSystem::drawBackground() {
  int width = 0;
  int height = 0;
  SDL_QueryTexture(background, nullptr, nullptr, &width, &height);
  SDL_Rect srcRect = {0, 0, width, height};
  SDL_Rect dstRect = {0, 0, width, height};
  SDL_RenderCopy(renderer, background, &srcRect, &dstRect);
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

VideoSystem::AnimationFrame VideoSystem::loadFrame(const std::string &path) {

  SDL_Surface *tempSurface = IMG_Load(path.c_str());
  if (!tempSurface) {
    throwError("Failed to load frame: " + path);
  }

  uint32_t colorKey = SDL_MapRGB(tempSurface->format, 85, 85, 85);
  SDL_SetColorKey(tempSurface, SDL_TRUE, colorKey);

  int width = tempSurface->w;
  int height = tempSurface->h;

  std::map<int, std::vector<uint8_t>> solidPixels;

  if (SDL_MUSTLOCK(tempSurface))
    SDL_LockSurface(tempSurface);

  uint8_t bytesPerPixel = tempSurface->format->BytesPerPixel;
  uint8_t *pixels = static_cast<uint8_t *>(tempSurface->pixels);

  for (int y = 0; y < height; ++y) {
    std::vector<uint8_t> row(width, 0);
    bool rowHasSolidPixels = false;

    for (int x = 0; x < width; ++x) {
      uint8_t *p = pixels + y * tempSurface->pitch + x * bytesPerPixel;
      uint32_t pixelData = 0;

      switch (bytesPerPixel) {
      case 1:
        pixelData = *p;
        break;
      case 2:
        pixelData = *reinterpret_cast<uint16_t *>(p);
        break;
      case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
          pixelData = p[0] << 16 | p[1] << 8 | p[2];
        else
          pixelData = p[0] | p[1] << 8 | p[2] << 16;
        break;
      case 4:
        pixelData = *reinterpret_cast<uint32_t *>(p);
        break;
      }

      uint8_t r, g, b, a;
      SDL_GetRGBA(pixelData, tempSurface->format, &r, &g, &b, &a);

      bool isTransparent = (r == 85 && g == 85 && b == 85) || (a == 0);

      if (!isTransparent) {
        row[x] = 1;
        rowHasSolidPixels = true;
      }
    }

    if (rowHasSolidPixels) {
      solidPixels[y] = row;
    }
  }

  if (SDL_MUSTLOCK(tempSurface))
    SDL_UnlockSurface(tempSurface);

  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, tempSurface);
  SDL_FreeSurface(tempSurface);

  auto hotspot = parseFrameHotspot(path);

  return AnimationFrame{tex,           width,          height,
                        hotspot.first, hotspot.second, solidPixels};
}

bool VideoSystem::checkPixelCollision(const std::string &nameA, int frameA,
                                      int xA, int yA, SDL_RendererFlip flipA,
                                      const std::string &nameB, int frameB,
                                      int xB, int yB, SDL_RendererFlip flipB) {
  if (animationStates.find(nameA) == animationStates.end() ||
      animationStates.find(nameB) == animationStates.end()) {
    return false;
  }

  const auto &animFrameA = animationStates.at(nameA)[frameA];
  const auto &animFrameB = animationStates.at(nameB)[frameB];

  int hotXA = (flipA & SDL_FLIP_HORIZONTAL)
                  ? (animFrameA.width - animFrameA.hotspotX)
                  : animFrameA.hotspotX;
  int hotYA = (flipA & SDL_FLIP_VERTICAL)
                  ? (animFrameA.height - animFrameA.hotspotY)
                  : animFrameA.hotspotY;
  SDL_Rect dstA = {xA - hotXA, yA - hotYA, animFrameA.width, animFrameA.height};

  int hotXB = (flipB & SDL_FLIP_HORIZONTAL)
                  ? (animFrameB.width - animFrameB.hotspotX)
                  : animFrameB.hotspotX;
  int hotYB = (flipB & SDL_FLIP_VERTICAL)
                  ? (animFrameB.height - animFrameB.hotspotY)
                  : animFrameB.hotspotY;
  SDL_Rect dstB = {xB - hotXB, yB - hotYB, animFrameB.width, animFrameB.height};

  SDL_Rect intersect;
  if (!SDL_IntersectRect(&dstA, &dstB, &intersect)) {
    return false;
  }

  for (int y = intersect.y; y < intersect.y + intersect.h; ++y) {
    int localYA = y - dstA.y;
    int localYB = y - dstB.y;

    if (flipA & SDL_FLIP_VERTICAL)
      localYA = animFrameA.height - 1 - localYA;
    if (flipB & SDL_FLIP_VERTICAL)
      localYB = animFrameB.height - 1 - localYB;

    auto rowItA = animFrameA.solidPixels.find(localYA);
    if (rowItA == animFrameA.solidPixels.end())
      continue;

    auto rowItB = animFrameB.solidPixels.find(localYB);
    if (rowItB == animFrameB.solidPixels.end())
      continue;

    const std::vector<uint8_t> &rowA = rowItA->second;
    const std::vector<uint8_t> &rowB = rowItB->second;

    for (int x = intersect.x; x < intersect.x + intersect.w; ++x) {
      int localXA = x - dstA.x;
      int localXB = x - dstB.x;

      if (flipA & SDL_FLIP_HORIZONTAL)
        localXA = animFrameA.width - 1 - localXA;
      if (flipB & SDL_FLIP_HORIZONTAL)
        localXB = animFrameB.width - 1 - localXB;

      if (rowA[localXA] && rowB[localXB]) {
        return true;
      }
    }
  }

  return false;
}

} // namespace openfranko::src::systems