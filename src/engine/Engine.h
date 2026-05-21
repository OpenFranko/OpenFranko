#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {

class Engine {
public:
  struct VirtualScreen {
    int id;
    int width, height;
    SDL_Texture *targetTexture;
  };

  struct AnimationFrame {
    SDL_Texture *texture;
    int width;
    int height;
    int hotspotX;
    int hotspotY;
  };

  static SDL_Event event;

  Engine();
  ~Engine();

  bool init(const std::string &title, int windowWidth, int windowHeight);

  bool initAudio();

  bool initTracker();

  bool loadS3M(const std::string &path);

  bool loadSFX(const std::string &path);

  void playMusic();
  void playSFX();

  void screenOpen(int screenId, int width, int height);
  void screen(int screenId);
  void loadAnimation(const std::string &name,
                     const std::vector<std::string> &framePaths);
  void hotspot(const std::string &name, size_t frameIndex, int x, int y);
  void bob(const std::string &name, int x, int y, int frame);
  void sprite(const std::string &name, int x, int y, int frame,
              SDL_RendererFlip flip = SDL_FLIP_NONE);
  void cls(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);
  void sync();

  bool loop();
  void shutdown();

private:
  SDL_Texture *LoadTexture(const char *fileName);

  AnimationFrame loadFrame(const std::string &path);

  std::map<std::string, std::vector<AnimationFrame>> animationStates;

  Mix_Music *trackerModule;
  Mix_Chunk *soundEffect;

  SDL_Window *window;
  SDL_Renderer *renderer;
  int currentScreenId;
  bool running;

  std::unordered_map<int, VirtualScreen> screens;
};

inline SDL_Event Engine::event;

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ENGINE_H_