#include "AudioSystem.h"
#include <stdexcept>

namespace openfranko::src::systems {
namespace {
void throwError(const std::string &cause) {
  throw std::runtime_error("Audio system could not be initialised!: " + cause);
}
} // namespace

AudioSystem::AudioSystem() {
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
  trackerModule = Mix_LoadMUS(path.c_str());
}

void AudioSystem::clearMusic() { Mix_FreeMusic(trackerModule); }

void AudioSystem::loadSFX(const std::string &name, const std::string &path) {
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
  Mix_FreeChunk(sfx);
  soundEffects.erase(name);
}

void AudioSystem::playMusic() { Mix_PlayMusic(trackerModule, -1); }

void AudioSystem::stopMusic() { Mix_HaltMusic(); }

void AudioSystem::playSFX(const std::string &name) {
  if (soundEffects.find(name) == soundEffects.end()) {
    return;
  }

  auto soundEffect = soundEffects.at(name);

  Mix_PlayChannel(-1, soundEffect, 0);
}

void AudioSystem::stopSFX() { Mix_HaltChannel(-1); }

} // namespace openfranko::src::systems