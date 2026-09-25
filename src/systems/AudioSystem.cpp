#include "AudioSystem.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace openfranko::src::systems {
namespace {

constexpr int OUTPUT_RATE = 22050;
constexpr int OUTPUT_FRAMES = 1024;
constexpr int PAL_VBL_RATE = 50;

} // namespace

AudioSystem::AudioSystem()
    : mixer(OUTPUT_RATE), vblRate(PAL_VBL_RATE),
      device(std::make_unique<AudioDevice>(OUTPUT_RATE, OUTPUT_FRAMES,
                                           [this](int16_t *stereo, int frames) {
                                             mixer.render(stereo, frames);
                                           })) {}

void AudioSystem::loadMusic(const std::string &path) {
  clearMusic();
  std::ifstream file(path, std::ios::binary);
  const std::vector<char> module{std::istreambuf_iterator<char>(file),
                                 std::istreambuf_iterator<char>()};
  if (mixer.loadModule(module)) {
    musicPath = path;
  }
}

void AudioSystem::clearMusic() {
  mixer.releaseModule();
  musicPath.clear();
}

const std::string &AudioSystem::loadedMusic() const { return musicPath; }

void AudioSystem::loadSFX(const std::string &name, const std::string &path) {
  clearSFX(name);
  try {
    sounds[name] = std::make_unique<Sound>(loadWave(path));
  } catch (const std::runtime_error &) {
    return;
  }
}

void AudioSystem::clearSFX(const std::string &name) {
  const auto sound = sounds.find(name);
  if (sound == sounds.end()) {
    return;
  }
  mixer.stop(*sound->second);
  sounds.erase(sound);
}

void AudioSystem::playMusic() {
  tempoScale = 1.0;
  mixer.startModule();
  applyTempo();
}

void AudioSystem::stopMusic() { mixer.stopModule(); }

void AudioSystem::setMusicVolume(int volume) { mixer.setMusicVolume(volume); }

void AudioSystem::setMusicTempoScale(double scale) {
  tempoScale = scale;
  applyTempo();
}

void AudioSystem::setVblRate(int hertz) {
  if (hertz == vblRate) {
    return;
  }
  vblRate = hertz;
  applyTempo();
}

void AudioSystem::setLowPassFilter(bool on) { mixer.setFilter(on); }

void AudioSystem::playSample(const std::string &name, int voiceMask) {
  const auto sound = sounds.find(name);
  if (sound != sounds.end()) {
    mixer.play(*sound->second, voiceMask, 0, sampleLooping);
  }
}

void AudioSystem::playSampleAt(const std::string &name, int voiceMask,
                               int frequency) {
  const auto sound = sounds.find(name);
  if (sound != sounds.end() && frequency > 0) {
    mixer.play(*sound->second, voiceMask, frequency, sampleLooping);
  }
}

void AudioSystem::setSampleLooping(bool looping) {
  sampleLooping = looping;
  if (!looping) {
    mixer.endLoops();
  }
}

void AudioSystem::stopSFX() { mixer.stopAll(); }

void AudioSystem::update() { mixer.update(); }

void AudioSystem::applyTempo() {
  mixer.setModuleTempo(static_cast<double>(PAL_VBL_RATE) /
                       (vblRate * tempoScale));
}

} // namespace openfranko::src::systems
