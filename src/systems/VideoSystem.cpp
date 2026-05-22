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

SDL_Texture *VideoSystem::loadTexture(const char *fileName) {
  SDL_Surface *tempSurface = IMG_Load(fileName);
  Uint32 colorKey = SDL_MapRGB(tempSurface->format, 85, 85, 85);

  SDL_SetColorKey(tempSurface, SDL_TRUE, colorKey);

  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, tempSurface);
  SDL_FreeSurface(tempSurface);
  return tex;
}

VideoSystem::AnimationFrame VideoSystem::loadFrame(const std::string &path) {
  SDL_Texture *tex = loadTexture(path.c_str());

  int width, height;
  SDL_QueryTexture(tex, nullptr, nullptr, &width, &height);
  auto hotspot = parseFrameHotspot(path);

  return AnimationFrame{tex, width, height, hotspot.first, hotspot.second};
}

} // namespace openfranko::src::systems