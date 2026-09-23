#ifndef SYSTEMS_AUDIOSYSTEM_H_
#define SYSTEMS_AUDIOSYSTEM_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <map>
#include <string>

namespace openfranko {
namespace src {
namespace systems {

class AudioSystem {
public:
  AudioSystem();
  ~AudioSystem();

  void loadMusic(const std::string &path);
  void clearMusic();
  void loadSFX(const std::string &name, const std::string &path);
  void clearSFX(const std::string &name);
  void playMusic();
  void stopMusic();
  void playSFX(const std::string &name);
  void stopSFX();

private:
  Mix_Music *trackerModule = nullptr;
  std::map<std::string, Mix_Chunk *> soundEffects;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIOSYSTEM_H_