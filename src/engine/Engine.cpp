#include "Engine.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>

namespace openfranko::src::engine {

namespace {

struct ParsedHotspot {
  int x;
  int y;
};

std::optional<ParsedHotspot> parseHotspotText(const std::string &text) {
  static const std::regex numberPattern(R"([-+]?\d+)");

  std::sregex_iterator it(text.begin(), text.end(), numberPattern);
  std::sregex_iterator end;

  if (it == end) {
    return std::nullopt;
  }

  try {
    int x = std::stoi(it->str());
    ++it;
    if (it == end) {
      return std::nullopt;
    }
    int y = std::stoi(it->str());
    return ParsedHotspot{x, y};
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

std::optional<ParsedHotspot> parseHotspotSidecar(const std::string &path) {
  const std::filesystem::path framePath(path);
  std::vector<std::filesystem::path> candidates;

  candidates.emplace_back(path + ".hotspot");

  auto stemSidecar = framePath;
  stemSidecar.replace_extension(".hotspot");
  candidates.push_back(stemSidecar);

  for (const auto &candidate : candidates) {
    if (!std::filesystem::is_regular_file(candidate)) {
      continue;
    }

    std::ifstream file(candidate);
    std::stringstream buffer;
    buffer << file.rdbuf();

    if (auto hotspot = parseHotspotText(buffer.str())) {
      return hotspot;
    }

    std::cerr << "Warning: Could not parse hotspot metadata " << candidate
              << "\n";
  }

  return std::nullopt;
}

std::optional<ParsedHotspot> parseHotspotFilename(const std::string &path) {
  std::string name = std::filesystem::path(path).stem().string();
  std::string lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  size_t marker = lower.find("hotspot");
  if (marker == std::string::npos) {
    marker = lower.find("hot");
  }
  if (marker == std::string::npos) {
    return std::nullopt;
  }

  return parseHotspotText(name.substr(marker));
}

ParsedHotspot parseFrameHotspot(const std::string &path) {
  if (auto hotspot = parseHotspotSidecar(path)) {
    return *hotspot;
  }
  if (auto hotspot = parseHotspotFilename(path)) {
    return *hotspot;
  }
  return ParsedHotspot{0, 0};
}

} // namespace

Engine::Engine()
    : window(nullptr), renderer(nullptr), currentScreenId(0), running(true) {}

Engine::~Engine() { shutdown(); }

bool Engine::init(const std::string &title, int windowWidth, int windowHeight) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    return false;
  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    return false;

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

  window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!window)
    return false;

  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer)
    return false;

  return true;
}

void Engine::screenOpen(int screenId, int width, int height) {
  SDL_Texture *target =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, width, height);

  screens[screenId] = {screenId, width, height, target};
  if (screens.size() == 1)
    screen(screenId);
}

void Engine::screen(int screenId) {
  if (screens.find(screenId) != screens.end()) {
    currentScreenId = screenId;
    SDL_SetRenderTarget(renderer, screens[currentScreenId].targetTexture);
  }
}

void Engine::loadAnimation(const std::string &name,
                           const std::vector<std::string> &framePaths) {
  std::vector<AnimationFrame> frames;

  for (auto &path : framePaths) {
    const auto frame = loadFrame(path);
    frames.emplace_back(frame);
  }

  animationStates.emplace(name, frames);
}

void Engine::hotspot(const std::string &name, size_t frameIndex, int x, int y) {
  if (animationStates.find(name) != animationStates.end()) {
    auto &frame = animationStates.at(name).at(frameIndex);
    frame.hotspotX = x;
    frame.hotspotY = y;
  }
}

void Engine::bob(const std::string &name, int x, int y, int frame) {
  if (animationStates.find(name) == animationStates.end())
    return;

  const auto frames = animationStates.at(name);
  const auto animFrame = frames.at(frame);

  SDL_Rect srcRect = {0, 0, animFrame.width, animFrame.height};
  SDL_Rect dstRect = {x - animFrame.hotspotX, y - animFrame.hotspotY,
                      animFrame.width, animFrame.height};

  SDL_RenderCopy(renderer, animFrame.texture, &srcRect, &dstRect);
}

void Engine::sprite(const std::string &name, int x, int y, int frame,
                    SDL_RendererFlip flip) {
  if (animationStates.find(name) == animationStates.end())
    return;

  const auto frames = animationStates.at(name);
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

void Engine::cls(uint8_t r, uint8_t g, uint8_t b) {
  SDL_SetRenderDrawColor(renderer, r, g, b, 255);
  SDL_RenderClear(renderer);
}

void Engine::sync() {
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

bool Engine::loop() {
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT)
      running = false;
  }
  return running;
}

void Engine::shutdown() {
  for (auto &state : animationStates) {
    for (auto &frame : state.second) {
      SDL_DestroyTexture(frame.texture);
    }
  }
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
  SDL_Quit();
}

SDL_Texture *Engine::LoadTexture(const char *fileName) {
  SDL_Surface *tempSurface = IMG_Load(fileName);
  Uint32 colorKey = SDL_MapRGB(tempSurface->format, 85, 85, 85);

  if (SDL_SetColorKey(tempSurface, SDL_TRUE, colorKey) < 0) {
    std::cerr << "Unable to set color key! SDL Error: " << SDL_GetError()
              << std::endl;
  }

  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, tempSurface);
  SDL_FreeSurface(tempSurface);
  return tex;
}

Engine::AnimationFrame Engine::loadFrame(const std::string &path) {
  SDL_Texture *tex = LoadTexture(path.c_str());
  if (!tex) {
    std::cerr << "Error: Could not load bitmap " << path << "\n";
    return {};
  }

  int width, height;
  SDL_QueryTexture(tex, nullptr, nullptr, &width, &height);
  auto hotspot = parseFrameHotspot(path);

  return AnimationFrame{tex, width, height, hotspot.x, hotspot.y};
}

} // namespace openfranko::src::engine
