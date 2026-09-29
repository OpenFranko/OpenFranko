#include "audio/AudioSystem.h"

#include "audio/AudioOutput.h"

#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace openfranko::src::systems::audio {
namespace {

constexpr int OUTPUT_RATE = 22050;
constexpr int OUTPUT_FRAMES = 1024;
constexpr int PAL_VBL_RATE = 50;

} // namespace

AudioSystem::AudioSystem(Read read)
    : m_output(std::make_unique<Output>(OUTPUT_RATE)) {
  m_output->read = std::move(read);
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
  std::vector<uint8_t> file;
  try {
    file = m_output->read(path);
  } catch (const std::runtime_error &) {
    return;
  }
  const std::vector<char> module(file.begin(), file.end());
  std::lock_guard<AudioDevice> lock(*m_output->device);
  if (m_output->mixer.loadModule(module)) {
    m_output->musicPath = path;
  }
}

void AudioSystem::clearMusic() {
  std::lock_guard<AudioDevice> lock(*m_output->device);
  m_output->mixer.releaseModule();
  m_output->musicPath.clear();
}

const std::string &AudioSystem::loadedMusic() const {
  return m_output->musicPath;
}

void AudioSystem::loadSample(const std::string &name, const std::string &path) {
  clearSample(name);
  try {
    m_output->sounds[name] =
        std::make_unique<Sound>(readWave(m_output->read(path)));
  } catch (const std::runtime_error &) {
    return;
  }
}

void AudioSystem::clearSample(const std::string &name) {
  const auto sound = m_output->sounds.find(name);
  if (sound == m_output->sounds.end()) {
    return;
  }
  std::lock_guard<AudioDevice> lock(*m_output->device);
  m_output->mixer.stop(*sound->second);
  m_output->sounds.erase(sound);
}

void AudioSystem::playMusic() { startMusic(true); }

void AudioSystem::playMusicOnce() { startMusic(false); }

void AudioSystem::stopMusic() {
  std::lock_guard<AudioDevice> lock(*m_output->device);
  m_output->mixer.stopModule();
}

void AudioSystem::setMusicVolume(int volume) {
  std::lock_guard<AudioDevice> lock(*m_output->device);
  m_output->mixer.setMusicVolume(volume);
}

void AudioSystem::setMusicTempoScale(double scale) {
  std::lock_guard<AudioDevice> lock(*m_output->device);
  m_output->tempoScale = scale;
  applyTempo();
}

void AudioSystem::setMusicTempo(int tempo) {
  std::lock_guard<AudioDevice> lock(*m_output->device);
  m_output->mixer.overrideModuleTempo(tempo);
}

void AudioSystem::setVblRate(int hertz) {
  if (hertz == m_output->vblRate) {
    return;
  }
  std::lock_guard<AudioDevice> lock(*m_output->device);
  m_output->vblRate = hertz;
  applyTempo();
}

void AudioSystem::setLowPassFilter(bool on) {
  std::lock_guard<AudioDevice> lock(*m_output->device);
  m_output->mixer.setFilter(on);
}

void AudioSystem::playSample(const std::string &name, int voiceMask) {
  const auto sound = m_output->sounds.find(name);
  if (sound != m_output->sounds.end()) {
    std::lock_guard<AudioDevice> lock(*m_output->device);
    m_output->mixer.play(*sound->second, voiceMask, 0, m_output->sampleLooping);
  }
}

void AudioSystem::playSampleAt(const std::string &name, int voiceMask,
                               int frequency) {
  const auto sound = m_output->sounds.find(name);
  if (sound != m_output->sounds.end() && frequency > 0) {
    std::lock_guard<AudioDevice> lock(*m_output->device);
    m_output->mixer.play(*sound->second, voiceMask, frequency,
                         m_output->sampleLooping);
  }
}

void AudioSystem::setSampleLooping(bool looping) {
  m_output->sampleLooping = looping;
  if (!looping) {
    std::lock_guard<AudioDevice> lock(*m_output->device);
    m_output->mixer.endLoops();
  }
}

void AudioSystem::stopSamples() {
  std::lock_guard<AudioDevice> lock(*m_output->device);
  m_output->mixer.stopAll();
}

void AudioSystem::update() {
  m_output->device->update();
  std::lock_guard<AudioDevice> lock(*m_output->device);
  m_output->mixer.update();
}

void AudioSystem::startMusic(bool looping) {
  std::lock_guard<AudioDevice> lock(*m_output->device);
  m_output->tempoScale = 1.0;
  m_output->mixer.startModule(looping);
  applyTempo();
}

void AudioSystem::applyTempo() {
  m_output->mixer.setModuleTempo(static_cast<double>(PAL_VBL_RATE) /
                                 (m_output->vblRate * m_output->tempoScale));
}

} // namespace openfranko::src::systems::audio
