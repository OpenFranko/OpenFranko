#ifndef SYSTEMS_AUDIOSYSTEM_H_
#define SYSTEMS_AUDIOSYSTEM_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <xmp.h>

namespace openfranko {
namespace src {
namespace systems {

class AudioSystem {
public:
  static constexpr int ALL_VOICES = 0xF;

  AudioSystem();
  ~AudioSystem();

  void loadMusic(const std::string &path);
  void clearMusic();
  const std::string &loadedMusic() const;
  void loadSFX(const std::string &name, const std::string &path);
  void clearSFX(const std::string &name);
  void playMusic();
  void stopMusic();
  void setMusicVolume(int volume);
  void setMusicTempoScale(double scale);
  void setVblRate(int hertz);
  void setLowPassFilter(bool on);
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

  struct Biquad {
    double b0 = 0.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
  };

  static void SDLCALL mixMusic(void *system, Uint8 *stream, int length);
  static void SDLCALL filterOutput(void *system, Uint8 *stream, int length);
  void stopPlayer();
  void applyTempo();
  void playChunk(Mix_Chunk *chunk, int voiceMask);
  Mix_Chunk *pitchedChunk(const std::string &name, int frequency);
  void clearPitchedChunks(const std::string &name);
  void applyMusicVolume();
  bool isVoicePlaying(const Mix_Chunk *chunk) const;
  uint32_t chunkMilliseconds(const Mix_Chunk *chunk) const;

  xmp_context player = nullptr;
  bool moduleLoaded = false;
  std::string musicPath;
  bool musicPlaying = false;
  std::mutex musicMutex;
  SDL_AudioStream *musicStream = nullptr;
  std::vector<Uint8> renderBuffer;
  std::vector<Uint8> mixBuffer;
  std::atomic<int> mixerVolume{0};
  double tempoScale = 1.0;
  int vblRate = 0;
  std::atomic<bool> filterOn{false};
  Biquad lowPass;
  std::vector<std::array<double, 4>> filterHistory;
  int deviceRate = 0;
  Uint16 deviceFormat = 0;
  int deviceChannels = 0;
  std::map<std::string, Mix_Chunk *> soundEffects;
  std::map<std::string, int> nativeRates;
  std::map<std::pair<std::string, int>, PitchedChunk> pitchedChunks;
  int musicVolume;
  const Mix_Chunk *silencingSample = nullptr;
  bool sampleLooping = false;
  std::array<uint32_t, 4> voiceStarted{};
  std::array<const Mix_Chunk *, 4> loopingVoices{};
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIOSYSTEM_H_