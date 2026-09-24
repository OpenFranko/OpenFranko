#include "AudioSystem.h"
#include <stdexcept>

namespace openfranko::src::systems {
namespace {

constexpr int MAX_AMOS_VOLUME = 64;
constexpr int DEFAULT_MUSIC_VOLUME = 56;
constexpr int VOICES = 4;
constexpr int ALL_VOICES = 0xF;
constexpr uint8_t FULL_PAN = 255;

bool isLeftVoice(int voice) { return voice == 0 || voice == 3; }

void throwError(const std::string &cause) {
  throw std::runtime_error("Audio system could not be initialised!: " + cause);
}
} // namespace

AudioSystem::AudioSystem() : musicVolume(DEFAULT_MUSIC_VOLUME) {
  setenv("MODPLUG_RESAMPLING_MODE", "1", 1);

  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    throwError("SDL");
  }

  if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
    throwError("OpenAudio");
  }

  int flags = MIX_INIT_MOD;
  int initted = Mix_Init(flags);
  if ((initted & flags) != flags) {
    throwError("Mix_Init");
  }

  Mix_ReserveChannels(VOICES);

  applyMusicVolume();
}

AudioSystem::~AudioSystem() {
  for (auto &soundEffect : soundEffects) {
    Mix_FreeChunk(soundEffect.second);
  }
  Mix_FreeMusic(trackerModule);
  Mix_Quit();
  Mix_CloseAudio();
}

void AudioSystem::loadMusic(const std::string &path) {
  clearMusic();
  trackerModule = Mix_LoadMUS(path.c_str());
}

void AudioSystem::clearMusic() {
  Mix_FreeMusic(trackerModule);
  trackerModule = nullptr;
}

void AudioSystem::loadSFX(const std::string &name, const std::string &path) {
  clearSFX(name);
  auto soundEffect = Mix_LoadWAV(path.c_str());
  if (soundEffect) {
    soundEffects.emplace(name, soundEffect);
  }
}

void AudioSystem::clearSFX(const std::string &name) {
  if (soundEffects.find(name) == soundEffects.end()) {
    return;
  }

  auto &sfx = soundEffects.at(name);
  if (sfx == silencingSample) {
    silencingSample = nullptr;
    applyMusicVolume();
  }
  Mix_FreeChunk(sfx);
  soundEffects.erase(name);
}

void AudioSystem::playMusic() {
  Mix_PlayMusic(trackerModule, -1);
  applyMusicVolume();
}

void AudioSystem::stopMusic() { Mix_HaltMusic(); }

void AudioSystem::setMusicVolume(int volume) {
  musicVolume = volume;
  applyMusicVolume();
}

void AudioSystem::playSFX(const std::string &name) {
  if (soundEffects.find(name) == soundEffects.end()) {
    return;
  }

  auto soundEffect = soundEffects.at(name);

  Mix_PlayChannel(-1, soundEffect, 0);
}

void AudioSystem::playSFXSilencingMusic(const std::string &name) {
  auto it = soundEffects.find(name);
  if (it == soundEffects.end()) {
    return;
  }

  const int channel = Mix_PlayChannel(-1, it->second, 0);
  if (channel < 0) {
    return;
  }
  silencingChannel = channel;
  applyMusicVolume();
}

void AudioSystem::playSample(const std::string &name, int voiceMask) {
  auto it = soundEffects.find(name);
  if (it == soundEffects.end()) {
    return;
  }

  for (int voice = 0; voice < VOICES; ++voice) {
    if (voiceMask & (1 << voice)) {
      Mix_HaltChannel(voice);
      Mix_SetPanning(voice, isLeftVoice(voice) ? FULL_PAN : 0,
                     isLeftVoice(voice) ? 0 : FULL_PAN);
      Mix_PlayChannel(voice, it->second, sampleLooping ? -1 : 0);
      voiceStarted[static_cast<std::size_t>(voice)] = SDL_GetTicks();
      loopingVoices[static_cast<std::size_t>(voice)] =
          sampleLooping ? it->second : nullptr;
    }
  }
  if ((voiceMask & ALL_VOICES) == ALL_VOICES) {
    silencingSample = it->second;
    applyMusicVolume();
  }
}

void AudioSystem::stopSFX() { Mix_HaltChannel(-1); }

void AudioSystem::setSampleLooping(bool looping) {
  sampleLooping = looping;
  if (looping) {
    return;
  }
  for (int voice = 0; voice < VOICES; ++voice) {
    const auto index = static_cast<std::size_t>(voice);
    const Mix_Chunk *chunk = loopingVoices[index];
    loopingVoices[index] = nullptr;
    if (!chunk || !Mix_Playing(voice) || Mix_GetChunk(voice) != chunk) {
      continue;
    }
    const uint32_t length = chunkMilliseconds(chunk);
    if (length == 0) {
      Mix_HaltChannel(voice);
      continue;
    }
    const uint32_t played = (SDL_GetTicks() - voiceStarted[index]) % length;
    Mix_ExpireChannel(voice, static_cast<int>(length - played));
  }
}

void AudioSystem::update() {
  if (silencingChannel && !Mix_Playing(*silencingChannel)) {
    silencingChannel.reset();
    applyMusicVolume();
  }
  if (silencingSample && !isVoicePlaying(silencingSample)) {
    silencingSample = nullptr;
    applyMusicVolume();
  }
}

void AudioSystem::applyMusicVolume() {
  const bool silenced = silencingChannel || silencingSample;
  Mix_VolumeMusic(silenced ? 0
                           : musicVolume * MIX_MAX_VOLUME / MAX_AMOS_VOLUME);
}

uint32_t AudioSystem::chunkMilliseconds(const Mix_Chunk *chunk) const {
  int frequency = 0;
  Uint16 format = 0;
  int channels = 0;
  if (Mix_QuerySpec(&frequency, &format, &channels) == 0) {
    return 0;
  }
  const uint64_t bytesPerSecond = static_cast<uint64_t>(frequency) *
                                  static_cast<uint64_t>(channels) *
                                  (SDL_AUDIO_BITSIZE(format) / 8);
  if (bytesPerSecond == 0) {
    return 0;
  }
  return static_cast<uint32_t>(static_cast<uint64_t>(chunk->alen) * 1000 /
                               bytesPerSecond);
}

bool AudioSystem::isVoicePlaying(const Mix_Chunk *chunk) const {
  for (int voice = 0; voice < VOICES; ++voice) {
    if (Mix_Playing(voice) && Mix_GetChunk(voice) == chunk) {
      return true;
    }
  }
  return false;
}

} // namespace openfranko::src::systems