#include "audio/AudioSystem.h"
#include "audio/AudioOutput.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace openfranko::src::systems::audio {
namespace {

constexpr int OUTPUT_RATE = 22050;
constexpr int OUTPUT_FRAMES = 1024;
constexpr int PAL_VBL_RATE = 50;

} // namespace

AudioSystem::AudioSystem() : m_output(std::make_unique<Output>(OUTPUT_RATE)) {
  m_output->vblRate = PAL_VBL_RATE;
  m_output->device = std::make_unique<AudioDevice>(
      OUTPUT_RATE, OUTPUT_FRAMES,
      [mixer = &m_output->mixer](int16_t *stereo, int frames) {
        mixer->render(stereo, frames);
      });
}

AudioSystem::~AudioSystem() = default;

void AudioSystem::loadMusic(const std::string &path) {
  clearMusic();
  std::ifstream file(path, std::ios::binary);
  const std::vector<char> module{std::istreambuf_iterator<char>(file),
                                 std::istreambuf_iterator<char>()};
  if (m_output->mixer.loadModule(module)) {
    m_output->musicPath = path;
  }
}

void AudioSystem::clearMusic() {
  m_output->mixer.releaseModule();
  m_output->musicPath.clear();
}

const std::string &AudioSystem::loadedMusic() const {
  return m_output->musicPath;
}

void AudioSystem::loadSample(const std::string &name, const std::string &path) {
  clearSample(name);
  try {
    m_output->sounds[name] = std::make_unique<Sound>(loadWave(path));
  } catch (const std::runtime_error &) {
    return;
  }
}

void AudioSystem::clearSample(const std::string &name) {
  const auto sound = m_output->sounds.find(name);
  if (sound == m_output->sounds.end()) {
    return;
  }
  m_output->mixer.stop(*sound->second);
  m_output->sounds.erase(sound);
}

void AudioSystem::playMusic() { startMusic(true); }

void AudioSystem::playMusicOnce() { startMusic(false); }

void AudioSystem::stopMusic() { m_output->mixer.stopModule(); }

void AudioSystem::setMusicVolume(int volume) {
  m_output->mixer.setMusicVolume(volume);
}

void AudioSystem::setMusicTempoScale(double scale) {
  m_output->tempoScale = scale;
  applyTempo();
}

void AudioSystem::setMusicTempo(int tempo) {
  m_output->mixer.overrideModuleTempo(tempo);
}

void AudioSystem::setVblRate(int hertz) {
  if (hertz == m_output->vblRate) {
    return;
  }
  m_output->vblRate = hertz;
  applyTempo();
}

void AudioSystem::setLowPassFilter(bool on) { m_output->mixer.setFilter(on); }

void AudioSystem::playSample(const std::string &name, int voiceMask) {
  const auto sound = m_output->sounds.find(name);
  if (sound != m_output->sounds.end()) {
    m_output->mixer.play(*sound->second, voiceMask, 0, m_output->sampleLooping);
  }
}

void AudioSystem::playSampleAt(const std::string &name, int voiceMask,
                               int frequency) {
  const auto sound = m_output->sounds.find(name);
  if (sound != m_output->sounds.end() && frequency > 0) {
    m_output->mixer.play(*sound->second, voiceMask, frequency,
                         m_output->sampleLooping);
  }
}

void AudioSystem::setSampleLooping(bool looping) {
  m_output->sampleLooping = looping;
  if (!looping) {
    m_output->mixer.endLoops();
  }
}

void AudioSystem::stopSamples() { m_output->mixer.stopAll(); }

void AudioSystem::update() { m_output->mixer.update(); }

void AudioSystem::startMusic(bool looping) {
  m_output->tempoScale = 1.0;
  m_output->mixer.startModule(looping);
  applyTempo();
}

void AudioSystem::applyTempo() {
  m_output->mixer.setModuleTempo(static_cast<double>(PAL_VBL_RATE) /
                                 (m_output->vblRate * m_output->tempoScale));
}

} // namespace openfranko::src::systems::audio
