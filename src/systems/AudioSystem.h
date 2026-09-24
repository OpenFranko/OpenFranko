#ifndef SYSTEMS_AUDIOSYSTEM_H_
#define SYSTEMS_AUDIOSYSTEM_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <map>
#include <optional>
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
  void setMusicVolume(int volume);
  void playSFX(const std::string &name);
  void playSFXSilencingMusic(const std::string &name);
  void playSample(const std::string &name, int voiceMask);
  void stopSFX();
  void update();

private:
  void applyMusicVolume();
  bool isVoicePlaying(const Mix_Chunk *chunk) const;

  Mix_Music *trackerModule = nullptr;
  std::map<std::string, Mix_Chunk *> soundEffects;
  int musicVolume;
  std::optional<int> silencingChannel;
  const Mix_Chunk *silencingSample = nullptr;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIOSYSTEM_H_