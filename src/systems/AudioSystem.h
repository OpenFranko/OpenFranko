#ifndef SYSTEMS_AUDIOSYSTEM_H_
#define SYSTEMS_AUDIOSYSTEM_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

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
  void playSampleAt(const std::string &name, int voiceMask, int frequency);
  void setSampleLooping(bool looping);
  void stopSFX();
  void update();

private:
  struct PitchedChunk {
    std::vector<Uint8> data;
    Mix_Chunk *chunk = nullptr;
  };

  void playChunk(Mix_Chunk *chunk, int voiceMask);
  Mix_Chunk *pitchedChunk(const std::string &name, int frequency);
  void clearPitchedChunks(const std::string &name);
  void applyMusicVolume();
  bool isVoicePlaying(const Mix_Chunk *chunk) const;
  uint32_t chunkMilliseconds(const Mix_Chunk *chunk) const;

  Mix_Music *trackerModule = nullptr;
  std::map<std::string, Mix_Chunk *> soundEffects;
  std::map<std::string, int> nativeRates;
  std::map<std::pair<std::string, int>, PitchedChunk> pitchedChunks;
  int musicVolume;
  std::optional<int> silencingChannel;
  const Mix_Chunk *silencingSample = nullptr;
  bool sampleLooping = false;
  std::array<uint32_t, 4> voiceStarted{};
  std::array<const Mix_Chunk *, 4> loopingVoices{};
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIOSYSTEM_H_