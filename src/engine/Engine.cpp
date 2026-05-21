#include "Engine.h"
#include <algorithm>
#include <cstdint>
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

uint16_t readLittleEndian16(const char *bytes) {
  return static_cast<uint16_t>(
      static_cast<uint8_t>(bytes[0]) |
      (static_cast<uint16_t>(static_cast<uint8_t>(bytes[1])) << 8));
}

std::optional<ParsedHotspot> parseHotspotBmpHeader(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return std::nullopt;
  }

  char header[10]{};
  file.read(header, sizeof(header));
  if (file.gcount() != sizeof(header) || header[0] != 'B' || header[1] != 'M') {
    return std::nullopt;
  }

  return ParsedHotspot{readLittleEndian16(header + 6),
                       readLittleEndian16(header + 8)};
}

ParsedHotspot parseFrameHotspot(const std::string &path) {
  if (auto hotspot = parseHotspotBmpHeader(path)) {
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

bool Engine::initAudio() {
  setenv("MODPLUG_RESAMPLING_MODE", "1", 1);

  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    return false;
  }

  if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
    std::cerr << "SDL_mixer could not initialize! Mix_Error: " << Mix_GetError()
              << std::endl;
    SDL_Quit();
    return false;
  }
  return true;
}

bool Engine::initTracker() {
  int flags = MIX_INIT_MOD;
  int initted = Mix_Init(flags);
  if ((initted & flags) != flags) {
    std::cerr << "Mix_Init: Failed to init required mod support!\n";
    std::cerr << "Mix_Error: " << Mix_GetError() << std::endl;
    Mix_CloseAudio();
    SDL_Quit();
    return false;
  }
  return true;
}

bool Engine::loadS3M(const std::string &path) {
  trackerModule = Mix_LoadMUS(path.c_str());
  if (!trackerModule) {
    std::cerr << "Failed to load S3M file! Mix_Error: " << Mix_GetError()
              << std::endl;
    return false;
  }
  return true;
}

bool Engine::loadSFX(const std::string &path) {
  soundEffect = Mix_LoadWAV(path.c_str());
  if (!soundEffect) {
    std::cerr << "Failed to load WAV! " << Mix_GetError() << std::endl;
    return false;
  }
  return true;
}

void Engine::playMusic() { Mix_PlayMusic(trackerModule, -1); }

void Engine::playSFX() { Mix_PlayChannel(-1, soundEffect, 0); }

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

  Mix_FreeChunk(soundEffect);
  Mix_FreeMusic(trackerModule);
  Mix_Quit();
  Mix_CloseAudio();

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
