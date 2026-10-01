#include "audio/AudioSystem.h"

#include "audio/Mixer.h"
#include "audio/S3m.h"
#include "audio/Tracker.h"
#include "audio/Wave.h"
#include "jaguar/Hardware.h"
#include "jaguar/RiscProgram.h"
#include "jaguar/Video.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace openfranko::src::systems::audio {
namespace {

namespace jaguar = systems::jaguar;

constexpr uint32_t CLOCK_DIVIDER = 18;
constexpr double NTSC_CLOCK = 26590906.0;
constexpr double PAL_CLOCK = 26593900.0;
constexpr double CLOCKS_PER_FRAME = 64.0;
constexpr double REFERENCE_RATE = 22050.0;
constexpr double TICK_SECONDS_PER_BPM = 2.5;
constexpr int PAL_VBL_RATE = 50;
constexpr double AMOS_TEMPO_PER_BPM = 4.0 / 5.0;
constexpr int DEFAULT_MUSIC_VOLUME = 56;
constexpr double LED_FILTER_HERTZ = 3275.0;
constexpr double BUTTERWORTH_Q = 0.7071067811865476;
constexpr double TWO_PI = 6.28318530717958647692;
constexpr double FILTER_ONE = 4096.0;
constexpr double STEP_ONE = 4294967296.0;
constexpr double FRAMES_ONE = 65536.0;
constexpr double PERIOD_RATE = Tracker::C4_PERIOD *
                               static_cast<double>(Tracker::C4_RATE) *
                               Tracker::PERIOD_ONE;

constexpr std::size_t GAIN = 0;
constexpr std::size_t FILTER = 1;
constexpr std::size_t TICK_WRITE = 2;
constexpr std::size_t FLUSH_SEQ = 3;
constexpr std::size_t FLUSH_INDEX = 4;
constexpr std::size_t LOOP_MASK = 5;
constexpr std::size_t SFX_SEQ = 6;
constexpr std::size_t SFX_COMMAND = 10;
constexpr std::size_t COEFFICIENTS = 42;
constexpr std::size_t TICK_READ = 47;
constexpr std::size_t SFX_ACTIVE = 48;
constexpr std::size_t BLOCKS = 49;
constexpr std::size_t TICKS = 64;
constexpr std::size_t COMMAND_LONGS = 8;
constexpr std::size_t TICK_LONGS = 40;
constexpr std::size_t TICK_VOICES = 8;
constexpr std::size_t VOICE_LONGS = 8;
constexpr uint32_t TICK_SLOTS = 32;
constexpr uint32_t TICKS_AHEAD = 24;
constexpr std::size_t SHARED_LONGS = TICKS + TICK_SLOTS * TICK_LONGS;
constexpr uint32_t COUNTER_MASK = 0xFFFF;
constexpr uint32_t TRIGGER = 1;
constexpr uint32_t ACTIVE = 2;
constexpr uint32_t ALL_LOOPS = 0xF;
constexpr uint32_t RIGHT_SIDE = 4;
constexpr int PENDING_UPDATES = 3;
constexpr long DSP_WAIT_POLLS = 400000;

alignas(8) volatile uint32_t shared[SHARED_LONGS];

using RowPosition = std::pair<int, int>;
using ModuleTiming = std::pair<int, int>;

constexpr RowPosition NO_POSITION{-1, -1};

bool isLeftVoice(int voice) { return voice == 0 || voice == 3; }

uint32_t counter(std::size_t at) { return shared[at] & COUNTER_MASK; }

struct VoiceUse {
  const Sound *sound = nullptr;
  int frequency = 0;
  int pending = 0;
};

struct MusicVoice {
  int sample = -1;
  uint32_t period = 0;
};

} // namespace

struct AudioSystem::Output {
  Read read;
  std::string musicPath;
  std::unique_ptr<S3mModule> module;
  std::unique_ptr<Tracker> tracker;
  std::vector<uint32_t> lengths;
  std::map<std::string, std::unique_ptr<Sound>> sounds;
  std::array<VoiceUse, Mixer::VOICES> voices;
  std::optional<VoiceUse> silencing;
  std::array<MusicVoice, S3mModule::CHANNELS> music;
  double rate = 0.0;
  uint64_t periodSteps = 0;
  double tempoScale = 1.0;
  int vblRate = PAL_VBL_RATE;
  double moduleTempoFactor = 1.0;
  double tickFactor = 1.0;
  int tempoOverride = 0;
  RowPosition overridePosition = NO_POSITION;
  RowPosition lastPosition = NO_POSITION;
  ModuleTiming overrideTiming{0, 0};
  RowPosition position = NO_POSITION;
  ModuleTiming timing{0, 0};
  int framesBpm = -1;
  uint32_t framesVersion = 0;
  uint32_t tickVersion = 1;
  uint32_t tickFrames = 0;
  bool playing = false;
  bool once = false;
  bool draining = false;
  int musicVolume = DEFAULT_MUSIC_VOLUME;
  bool sampleLooping = false;
  bool filter = false;
  uint32_t ticksWritten = 0;
  uint32_t flushes = 0;
  std::array<uint32_t, Mixer::VOICES> commands{};
  jaguar::RiscProgram dsp;

  void startDsp();
  void stopDsp();
  void waitForDsp();
  void flushMusic();
  void applyGain();
  void applyModuleTempo();
  void followModuleTempo();
  void produceTicks();
  void writeTick(const TrackerTick &tick);
  uint32_t framesFor(int bpm);
  bool isActive(int voice) const;
  bool isSounding(const VoiceUse &use) const;
  void command(int voice, const Sound *sound, int frequency, bool loop);
  void play(const std::string &name, int voiceMask, int frequency);
};

void AudioSystem::Output::startDsp() {
  const bool ntsc = jaguar::detectGeometry().ntsc;
  rate = (ntsc ? NTSC_CLOCK : PAL_CLOCK) /
         (CLOCKS_PER_FRAME * (CLOCK_DIVIDER + 1));
  periodSteps = static_cast<uint64_t>(PERIOD_RATE * STEP_ONE / rate);
  for (volatile uint32_t &value : shared) {
    value = 0;
  }
  shared[GAIN] = DEFAULT_MUSIC_VOLUME;
  shared[LOOP_MASK] = ALL_LOOPS;
  const double w0 = TWO_PI * LED_FILTER_HERTZ / rate;
  const double alpha = std::sin(w0) / (2.0 * BUTTERWORTH_Q);
  const double cosine = std::cos(w0);
  const double a0 = 1.0 + alpha;
  const double coefficients[] = {(1.0 - cosine) / 2.0 / a0, (1.0 - cosine) / a0,
                                 (1.0 - cosine) / 2.0 / a0, 2.0 * cosine / a0,
                                 -(1.0 - alpha) / a0};
  for (std::size_t index = 0; index < 5; ++index) {
    const double scaled = coefficients[index] * FILTER_ONE;
    shared[COEFFICIENTS + index] = static_cast<uint32_t>(static_cast<int32_t>(
        scaled >= 0.0 ? std::floor(scaled + 0.5) : -std::floor(0.5 - scaled)));
  }
  jaguar::word(jaguar::JOYSTICK) = jaguar::JOYSTICK_AUDIO_ON;
  dsp = jaguar::dspProgram();
  jaguar::longWord(jaguar::DSP_CTRL) = 0;
  jaguar::loadProgram(dsp);
  jaguar::longWord(dsp.entries[1]) = reinterpret_cast<uint32_t>(shared);
  jaguar::longWord(dsp.entries[1] + 4) = CLOCK_DIVIDER;
  jaguar::longWord(dsp.entries[1] + 8) =
      static_cast<uint32_t>(periodSteps >> 32);
  jaguar::longWord(dsp.entries[1] + 12) = static_cast<uint32_t>(periodSteps);
  jaguar::longWord(jaguar::DSP_PC) = dsp.entries[0];
  jaguar::longWord(jaguar::DSP_CTRL) = jaguar::RISC_GO;
}

void AudioSystem::Output::stopDsp() {
  jaguar::longWord(jaguar::DSP_CTRL) = 0;
  jaguar::longWord(jaguar::DSP_LEFT) = 0;
  jaguar::longWord(jaguar::DSP_RIGHT) = 0;
}

void AudioSystem::Output::waitForDsp() {
  const uint32_t start = counter(BLOCKS);
  for (long poll = 0; poll < DSP_WAIT_POLLS; ++poll) {
    if (((counter(BLOCKS) - start) & COUNTER_MASK) >= 2) {
      return;
    }
  }
}

void AudioSystem::Output::flushMusic() {
  shared[FLUSH_INDEX] = ticksWritten;
  shared[FLUSH_SEQ] = ++flushes;
  draining = false;
}

void AudioSystem::Output::applyGain() {
  shared[GAIN] = static_cast<uint32_t>(silencing ? 0 : musicVolume);
}

void AudioSystem::Output::applyModuleTempo() {
  if (!playing) {
    return;
  }
  double factor = moduleTempoFactor;
  if (tempoOverride > 0) {
    overrideTiming = timing;
    const auto [speed, bpm] = overrideTiming;
    if (speed > 0 && bpm > 0) {
      factor *= AMOS_TEMPO_PER_BPM * bpm / (speed * tempoOverride);
    }
  }
  tickFactor = factor;
  ++tickVersion;
}

void AudioSystem::Output::followModuleTempo() {
  if (tempoOverride == 0) {
    return;
  }
  const bool loopedBack =
      lastPosition != NO_POSITION && position < lastPosition;
  if (!loopedBack && position != overridePosition &&
      module->tempoRows.count(position) > 0) {
    tempoOverride = 0;
    applyModuleTempo();
    overridePosition = position;
    lastPosition = position;
    return;
  }
  overridePosition = position;
  lastPosition = position;
  if (timing != overrideTiming) {
    applyModuleTempo();
  }
}

uint32_t AudioSystem::Output::framesFor(int bpm) {
  if (bpm != framesBpm || tickVersion != framesVersion) {
    framesBpm = bpm;
    framesVersion = tickVersion;
    const double reference =
        std::floor(REFERENCE_RATE * TICK_SECONDS_PER_BPM * tickFactor / bpm);
    tickFrames =
        static_cast<uint32_t>(reference * rate / REFERENCE_RATE * FRAMES_ONE);
  }
  return tickFrames;
}

void AudioSystem::Output::writeTick(const TrackerTick &tick) {
  volatile uint32_t *record =
      shared + TICKS + (ticksWritten % TICK_SLOTS) * TICK_LONGS;
  record[0] = framesFor(tick.bpm);
  for (std::size_t index = 0; index < S3mModule::CHANNELS; ++index) {
    const TrackerVoice &voice = tick.voices[index];
    MusicVoice &state = music[index];
    volatile uint32_t *slot = record + TICK_VOICES + index * VOICE_LONGS;
    uint32_t flags = voice.active ? ACTIVE : 0;
    if (voice.trigger && voice.sample >= 0) {
      flags |= TRIGGER;
      state.sample = voice.sample;
    }
    if (state.sample < 0) {
      slot[0] = 0;
      continue;
    }
    const S3mSample &sample =
        module->samples[static_cast<std::size_t>(state.sample)];
    const uint32_t start = reinterpret_cast<uint32_t>(sample.data.data());
    if (voice.period > 0) {
      state.period = voice.period;
    }
    slot[0] = flags;
    slot[1] = start;
    if (sample.looped) {
      slot[2] = start + sample.loopEnd;
      slot[3] = sample.loopEnd - sample.loopStart;
    } else {
      slot[2] = start + lengths[static_cast<std::size_t>(state.sample)];
      slot[3] = 0;
    }
    slot[4] = state.period;
    slot[5] = 0;
    slot[6] = static_cast<uint32_t>(voice.gainLeft) << 16 |
              static_cast<uint32_t>(voice.gainRight);
  }
  shared[TICK_WRITE] = ++ticksWritten;
}

void AudioSystem::Output::produceTicks() {
  if (!playing || !tracker) {
    return;
  }
  while (!draining &&
         ((ticksWritten - counter(TICK_READ)) & COUNTER_MASK) < TICKS_AHEAD) {
    if (once && tracker->loops() > 0) {
      draining = true;
      break;
    }
    const TrackerTick &tick = tracker->advance();
    position = {tick.pattern, tick.row};
    timing = {tick.speed, tick.bpm};
    writeTick(tick);
    followModuleTempo();
  }
  if (draining && ((ticksWritten - counter(TICK_READ)) & COUNTER_MASK) == 0) {
    playing = false;
    draining = false;
    tempoOverride = 0;
    flushMusic();
  }
}

bool AudioSystem::Output::isActive(int voice) const {
  return (counter(SFX_ACTIVE) & (1u << voice)) != 0 ||
         voices[static_cast<std::size_t>(voice)].pending > 0;
}

bool AudioSystem::Output::isSounding(const VoiceUse &use) const {
  for (int voice = 0; voice < Mixer::VOICES; ++voice) {
    const VoiceUse &current = voices[static_cast<std::size_t>(voice)];
    if (current.sound == use.sound && current.frequency == use.frequency &&
        isActive(voice)) {
      return true;
    }
  }
  return false;
}

void AudioSystem::Output::command(int voice, const Sound *sound, int frequency,
                                  bool loop) {
  volatile uint32_t *slot = shared + SFX_COMMAND + voice * COMMAND_LONGS;
  VoiceUse &use = voices[static_cast<std::size_t>(voice)];
  const int playRate = sound ? (frequency > 0 ? frequency : sound->rate) : 0;
  if (!sound || sound->frames.empty() || playRate <= 0) {
    use = VoiceUse{};
    slot[0] = 0;
    slot[1] = 0;
  } else {
    use = VoiceUse{sound, frequency, PENDING_UPDATES};
    const uint32_t start = reinterpret_cast<uint32_t>(sound->frames.data());
    const uint32_t length = static_cast<uint32_t>(sound->frames.size());
    const uint64_t step =
        static_cast<uint64_t>(static_cast<double>(playRate) / rate * STEP_ONE);
    slot[0] = start;
    slot[1] = start + length;
    slot[2] = loop ? length : 0;
    slot[3] = static_cast<uint32_t>(step >> 32);
    slot[4] = static_cast<uint32_t>(step);
    slot[5] = isLeftVoice(voice) ? 0 : RIGHT_SIDE;
  }
  shared[SFX_SEQ + static_cast<std::size_t>(voice)] =
      ++commands[static_cast<std::size_t>(voice)];
}

AudioSystem::AudioSystem(Read read) : m_output(std::make_unique<Output>()) {
  m_output->read = std::move(read);
  m_output->startDsp();
}

AudioSystem::~AudioSystem() { m_output->stopDsp(); }

void AudioSystem::loadMusic(const std::string &path) {
  clearMusic();
  std::vector<uint8_t> file;
  try {
    file = m_output->read(path);
  } catch (const std::runtime_error &) {
    return;
  }
  auto module = std::make_unique<S3mModule>();
  if (!parseS3m(file, *module)) {
    return;
  }
  m_output->lengths.clear();
  for (S3mSample &sample : module->samples) {
    m_output->lengths.push_back(static_cast<uint32_t>(sample.data.size()));
    if (!sample.data.empty()) {
      sample.data.push_back(sample.looped ? sample.data[sample.loopStart]
                                          : static_cast<int8_t>(0));
    }
  }
  m_output->module = std::move(module);
  m_output->tracker = std::make_unique<Tracker>(*m_output->module);
  m_output->musicPath = path;
}

void AudioSystem::clearMusic() {
  stopMusic();
  m_output->waitForDsp();
  m_output->tracker.reset();
  m_output->module.reset();
  m_output->lengths.clear();
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
  bool stopped = false;
  for (int voice = 0; voice < Mixer::VOICES; ++voice) {
    if (m_output->voices[static_cast<std::size_t>(voice)].sound ==
        sound->second.get()) {
      m_output->command(voice, nullptr, 0, false);
      stopped = true;
    }
  }
  if (m_output->silencing &&
      m_output->silencing->sound == sound->second.get()) {
    m_output->silencing.reset();
    m_output->applyGain();
  }
  if (stopped) {
    m_output->waitForDsp();
  }
  m_output->sounds.erase(sound);
}

void AudioSystem::playMusic() { startMusic(true); }

void AudioSystem::playMusicOnce() { startMusic(false); }

void AudioSystem::stopMusic() {
  m_output->playing = false;
  m_output->draining = false;
  m_output->tempoOverride = 0;
  m_output->overridePosition = NO_POSITION;
  m_output->lastPosition = NO_POSITION;
  m_output->flushMusic();
}

void AudioSystem::setMusicVolume(int volume) {
  m_output->musicVolume = volume;
  m_output->applyGain();
}

void AudioSystem::setMusicTempoScale(double scale) {
  m_output->tempoScale = scale;
  applyTempo();
}

void AudioSystem::setMusicTempo(int tempo) {
  Output &output = *m_output;
  if (!output.playing || tempo <= 0) {
    return;
  }
  output.tempoOverride = tempo;
  output.overridePosition = output.position;
  output.lastPosition = output.position;
  output.applyModuleTempo();
}

void AudioSystem::setVblRate(int hertz) {
  if (hertz == m_output->vblRate) {
    return;
  }
  m_output->vblRate = hertz;
  applyTempo();
}

void AudioSystem::setLowPassFilter(bool on) {
  m_output->filter = on;
  shared[FILTER] = on ? 1 : 0;
}

void AudioSystem::playSample(const std::string &name, int voiceMask) {
  m_output->play(name, voiceMask, 0);
}

void AudioSystem::playSampleAt(const std::string &name, int voiceMask,
                               int frequency) {
  if (frequency > 0) {
    m_output->play(name, voiceMask, frequency);
  }
}

void AudioSystem::Output::play(const std::string &name, int voiceMask,
                               int frequency) {
  const auto sound = sounds.find(name);
  if (sound == sounds.end()) {
    return;
  }
  for (int voice = 0; voice < Mixer::VOICES; ++voice) {
    if (voiceMask & (1 << voice)) {
      command(voice, sound->second.get(), frequency, sampleLooping);
    }
  }
  if ((voiceMask & Mixer::ALL_VOICES) == Mixer::ALL_VOICES) {
    silencing = VoiceUse{sound->second.get(), frequency, 0};
    applyGain();
  }
}

void AudioSystem::setSampleLooping(bool looping) {
  m_output->sampleLooping = looping;
  shared[LOOP_MASK] = looping ? ALL_LOOPS : 0;
}

void AudioSystem::stopSamples() {
  for (int voice = 0; voice < Mixer::VOICES; ++voice) {
    m_output->command(voice, nullptr, 0, false);
  }
}

void AudioSystem::update() {
  Output &output = *m_output;
  for (VoiceUse &use : output.voices) {
    if (use.pending > 0) {
      --use.pending;
    }
  }
  if (output.silencing && !output.isSounding(*output.silencing)) {
    output.silencing.reset();
    output.applyGain();
  }
  output.produceTicks();
}

void AudioSystem::startMusic(bool looping) {
  Output &output = *m_output;
  if (!output.tracker) {
    return;
  }
  output.flushMusic();
  output.tracker->restart();
  output.music.fill(MusicVoice{});
  output.playing = !output.tracker->isEmpty();
  output.once = !looping;
  output.tempoScale = 1.0;
  output.tempoOverride = 0;
  output.overridePosition = NO_POSITION;
  output.lastPosition = NO_POSITION;
  output.position = NO_POSITION;
  output.timing = {output.module->speed, output.module->tempo};
  applyTempo();
  output.produceTicks();
}

void AudioSystem::applyTempo() {
  Output &output = *m_output;
  output.moduleTempoFactor =
      static_cast<double>(PAL_VBL_RATE) / (output.vblRate * output.tempoScale);
  output.tickFactor = output.moduleTempoFactor;
  ++output.tickVersion;
  output.applyModuleTempo();
}

} // namespace openfranko::src::systems::audio
