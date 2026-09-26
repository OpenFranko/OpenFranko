#include "AudioSystem.h"
#include "AudioOutput.h"

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

AudioSystem::AudioSystem() : output(std::make_unique<Output>(OUTPUT_RATE)) {
  output->vblRate = PAL_VBL_RATE;
  output->device = std::make_unique<AudioDevice>(
      OUTPUT_RATE, OUTPUT_FRAMES,
      [mixer = &output->mixer](int16_t *stereo, int frames) {
        mixer->render(stereo, frames);
      });
}

AudioSystem::~AudioSystem() = default;

void AudioSystem::loadMusic(const std::string &path) {
  clearMusic();
  std::ifstream file(path, std::ios::binary);
  const std::vector<char> module{std::istreambuf_iterator<char>(file),
                                 std::istreambuf_iterator<char>()};
  if (output->mixer.loadModule(module)) {
    output->musicPath = path;
  }
}

void AudioSystem::clearMusic() {
  output->mixer.releaseModule();
  output->musicPath.clear();
}

const std::string &AudioSystem::loadedMusic() const {
  return output->musicPath;
}

void AudioSystem::loadSFX(const std::string &name, const std::string &path) {
  clearSFX(name);
  try {
    output->sounds[name] = std::make_unique<Sound>(loadWave(path));
  } catch (const std::runtime_error &) {
    return;
  }
}

void AudioSystem::clearSFX(const std::string &name) {
  const auto sound = output->sounds.find(name);
  if (sound == output->sounds.end()) {
    return;
  }
  output->mixer.stop(*sound->second);
  output->sounds.erase(sound);
}

void AudioSystem::playMusic() { startMusic(true); }

void AudioSystem::playMusicOnce() { startMusic(false); }

void AudioSystem::stopMusic() { output->mixer.stopModule(); }

void AudioSystem::setMusicVolume(int volume) {
  output->mixer.setMusicVolume(volume);
}

void AudioSystem::setMusicTempoScale(double scale) {
  output->tempoScale = scale;
  applyTempo();
}

void AudioSystem::setMusicTempo(int tempo) {
  output->mixer.overrideModuleTempo(tempo);
}

void AudioSystem::setVblRate(int hertz) {
  if (hertz == output->vblRate) {
    return;
  }
  output->vblRate = hertz;
  applyTempo();
}

void AudioSystem::setLowPassFilter(bool on) { output->mixer.setFilter(on); }

void AudioSystem::playSample(const std::string &name, int voiceMask) {
  const auto sound = output->sounds.find(name);
  if (sound != output->sounds.end()) {
    output->mixer.play(*sound->second, voiceMask, 0, output->sampleLooping);
  }
}

void AudioSystem::playSampleAt(const std::string &name, int voiceMask,
                               int frequency) {
  const auto sound = output->sounds.find(name);
  if (sound != output->sounds.end() && frequency > 0) {
    output->mixer.play(*sound->second, voiceMask, frequency,
                       output->sampleLooping);
  }
}

void AudioSystem::setSampleLooping(bool looping) {
  output->sampleLooping = looping;
  if (!looping) {
    output->mixer.endLoops();
  }
}

void AudioSystem::stopSFX() { output->mixer.stopAll(); }

void AudioSystem::update() { output->mixer.update(); }

void AudioSystem::startMusic(bool looping) {
  output->tempoScale = 1.0;
  output->mixer.startModule(looping);
  applyTempo();
}

void AudioSystem::applyTempo() {
  output->mixer.setModuleTempo(static_cast<double>(PAL_VBL_RATE) /
                               (output->vblRate * output->tempoScale));
}

} // namespace openfranko::src::systems
