#include "audio/Mixer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <xmp.h>

namespace openfranko::src::systems::audio {
namespace {

constexpr int DEFAULT_MUSIC_VOLUME = 56;
constexpr int STEREO = 2;
constexpr int VOICES_PER_SIDE = 2;
constexpr int FRACTION_BITS = 32;
constexpr int BYTE_SCALE = 256;
constexpr double LED_FILTER_HERTZ = 3275.0;
constexpr double BUTTERWORTH_Q = 0.7071067811865476;
constexpr double PI = 3.14159265358979323846;

constexpr double AMOS_TEMPO_PER_BPM = 4.0 / 5.0;
constexpr std::size_t TRACKED_SAMPLES = 256 * STEREO;

constexpr std::size_t S3M_ORDER_COUNT = 0x20;
constexpr std::size_t S3M_INSTRUMENT_COUNT = 0x22;
constexpr std::size_t S3M_PATTERN_COUNT = 0x24;
constexpr std::size_t S3M_MAGIC = 0x2C;
constexpr std::size_t S3M_ORDERS = 0x60;
constexpr std::size_t S3M_PARAGRAPH = 16;
constexpr std::size_t S3M_LENGTH_SIZE = 2;
constexpr int S3M_ROWS = 64;
constexpr uint8_t S3M_NOTE = 0x20;
constexpr uint8_t S3M_VOLUME = 0x40;
constexpr uint8_t S3M_COMMAND = 0x80;
constexpr uint8_t S3M_SET_SPEED = 1;
constexpr uint8_t S3M_SET_TEMPO = 20;

bool isLeftVoice(int voice) { return voice == 0 || voice == 3; }

std::set<std::pair<int, int>> s3mTempoRows(const std::vector<char> &data) {
  const auto byteAt = [&data](std::size_t at) -> std::size_t {
    return at < data.size() ? static_cast<uint8_t>(data[at]) : 0;
  };
  const auto wordAt = [&byteAt](std::size_t at) {
    return byteAt(at) | byteAt(at + 1) << 8;
  };
  std::set<std::pair<int, int>> rows;
  if (data.size() < S3M_ORDERS ||
      std::memcmp(data.data() + S3M_MAGIC, "SCRM", 4) != 0) {
    return rows;
  }
  const std::size_t pointers = S3M_ORDERS + wordAt(S3M_ORDER_COUNT) +
                               S3M_LENGTH_SIZE * wordAt(S3M_INSTRUMENT_COUNT);
  const std::size_t patterns = wordAt(S3M_PATTERN_COUNT);
  for (std::size_t pattern = 0; pattern < patterns; ++pattern) {
    const std::size_t start =
        wordAt(pointers + S3M_LENGTH_SIZE * pattern) * S3M_PARAGRAPH;
    if (start == 0) {
      continue;
    }
    const std::size_t end = std::min(data.size(), start + wordAt(start));
    std::size_t at = start + S3M_LENGTH_SIZE;
    int row = 0;
    while (row < S3M_ROWS && at < end) {
      const std::size_t what = byteAt(at++);
      if (what == 0) {
        ++row;
        continue;
      }
      if (what & S3M_NOTE) {
        at += 2;
      }
      if (what & S3M_VOLUME) {
        ++at;
      }
      if (what & S3M_COMMAND) {
        const std::size_t command = byteAt(at);
        if (command == S3M_SET_SPEED || command == S3M_SET_TEMPO) {
          rows.insert({static_cast<int>(pattern), row});
        }
        at += 2;
      }
    }
  }
  return rows;
}

int16_t clampSample(long value) {
  return static_cast<int16_t>(
      std::clamp(value, static_cast<long>(std::numeric_limits<int16_t>::min()),
                 static_cast<long>(std::numeric_limits<int16_t>::max())));
}

} // namespace

struct Mixer::Module {
  Module() : player(xmp_create_context()) {}
  ~Module() {
    if (player) {
      xmp_free_context(player);
    }
  }

  Module(const Module &) = delete;
  Module &operator=(const Module &) = delete;

  xmp_context player;
};

Mixer::Mixer(int outputRate)
    : m_rate(outputRate), m_module(std::make_unique<Module>()),
      m_musicVolume(DEFAULT_MUSIC_VOLUME) {
  if (!m_module->player) {
    throw std::runtime_error("Mixer error: no module player");
  }
  const double w0 = 2.0 * PI * LED_FILTER_HERTZ / m_rate;
  const double alpha = std::sin(w0) / (2.0 * BUTTERWORTH_Q);
  const double cosine = std::cos(w0);
  const double a0 = 1.0 + alpha;
  m_lowPass = {(1.0 - cosine) / 2.0 / a0, (1.0 - cosine) / a0,
               (1.0 - cosine) / 2.0 / a0, -2.0 * cosine / a0,
               (1.0 - alpha) / a0};
}

Mixer::~Mixer() {
  stopPlayer();
  if (m_moduleLoaded) {
    xmp_release_module(m_module->player);
  }
}

bool Mixer::loadModule(const std::vector<char> &data) {
  std::lock_guard<std::mutex> lock(m_mutex);
  stopPlayer();
  if (m_moduleLoaded) {
    xmp_release_module(m_module->player);
  }
  m_moduleLoaded = !data.empty() && xmp_load_module_from_memory(
                                        m_module->player, data.data(),
                                        static_cast<long>(data.size())) == 0;
  m_tempoRows = m_moduleLoaded ? s3mTempoRows(data) : std::set<RowPosition>{};
  return m_moduleLoaded;
}

void Mixer::releaseModule() {
  std::lock_guard<std::mutex> lock(m_mutex);
  stopPlayer();
  if (m_moduleLoaded) {
    xmp_release_module(m_module->player);
    m_moduleLoaded = false;
  }
  m_tempoRows.clear();
}

void Mixer::startModule(bool looping) {
  std::lock_guard<std::mutex> lock(m_mutex);
  stopPlayer();
  m_moduleLoops = looping ? 0 : 1;
  if (m_moduleLoaded && xmp_start_player(m_module->player, m_rate, 0) == 0) {
    m_modulePlaying = true;
  }
}

void Mixer::stopModule() {
  std::lock_guard<std::mutex> lock(m_mutex);
  stopPlayer();
}

bool Mixer::isModulePlaying() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_modulePlaying;
}

void Mixer::setModuleTempo(double factor) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_moduleTempoFactor = factor;
  applyModuleTempo();
}

void Mixer::overrideModuleTempo(int tempo) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_modulePlaying || tempo <= 0) {
    return;
  }
  m_tempoOverride = tempo;
  m_overridePosition = modulePosition();
  m_lastPosition = m_overridePosition;
  applyModuleTempo();
}

bool Mixer::isModuleTempoOverridden() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_tempoOverride > 0;
}

void Mixer::setMusicVolume(int volume) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_musicVolume = volume;
}

void Mixer::setFilter(bool on) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_filterOn = on;
}

void Mixer::play(const Sound &sound, int voiceMask, int frequency, bool loop) {
  std::lock_guard<std::mutex> lock(m_mutex);
  const int playRate = frequency > 0 ? frequency : sound.rate;
  for (int voice = 0; voice < VOICES; ++voice) {
    if ((voiceMask & (1 << voice)) == 0) {
      continue;
    }
    Voice &target = m_voices[static_cast<std::size_t>(voice)];
    target = Voice{};
    if (!sound.frames.empty() && playRate > 0) {
      target.sound = &sound;
      target.frequency = frequency;
      target.step = (static_cast<uint64_t>(playRate) << FRACTION_BITS) /
                    static_cast<uint64_t>(m_rate);
      target.loop = loop;
    }
  }
  if ((voiceMask & ALL_VOICES) == ALL_VOICES) {
    m_silencing = Playing{&sound, frequency};
  }
}

void Mixer::endLoops() {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (Voice &voice : m_voices) {
    voice.loop = false;
  }
}

void Mixer::stop(const Sound &sound) {
  std::lock_guard<std::mutex> lock(m_mutex);
  for (Voice &voice : m_voices) {
    if (voice.sound == &sound) {
      voice = Voice{};
    }
  }
  if (m_silencing && m_silencing->sound == &sound) {
    m_silencing.reset();
  }
}

void Mixer::stopAll() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_voices.fill(Voice{});
}

void Mixer::update() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_silencing && !isSounding(*m_silencing)) {
    m_silencing.reset();
  }
}

bool Mixer::isPlaying(int voice) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_voices.at(static_cast<std::size_t>(voice)).sound != nullptr;
}

bool Mixer::isMusicSilenced() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_silencing.has_value();
}

void Mixer::render(int16_t *stereo, int frames) {
  std::lock_guard<std::mutex> lock(m_mutex);
  const std::size_t samples = static_cast<std::size_t>(frames) * STEREO;
  m_musicBuffer.assign(samples, 0);
  if (m_modulePlaying && !playModule(samples) && m_moduleLoops > 0) {
    stopPlayer();
  }

  const int musicLevel = m_silencing ? 0 : m_musicVolume;
  for (std::size_t frame = 0; frame < static_cast<std::size_t>(frames);
       ++frame) {
    int16_t *out = stereo + frame * STEREO;
    for (std::size_t channel = 0; channel < STEREO; ++channel) {
      out[channel] = clampSample(m_musicBuffer[frame * STEREO + channel] *
                                 musicLevel / MAX_VOLUME);
    }
    for (int voice = 0; voice < VOICES; ++voice) {
      Voice &playing = m_voices[static_cast<std::size_t>(voice)];
      if (!playing.sound) {
        continue;
      }
      const int sample =
          nextSample(playing) * SAMPLE_VOLUME / MAX_VOLUME / VOICES_PER_SIDE;
      int16_t &side = out[isLeftVoice(voice) ? 0 : 1];
      side = clampSample(side + sample);
    }
  }
  filter(stereo, frames);
}

void Mixer::stopPlayer() {
  m_tempoOverride = 0;
  m_overridePosition = {-1, -1};
  m_lastPosition = {-1, -1};
  if (m_modulePlaying) {
    xmp_end_player(m_module->player);
    m_modulePlaying = false;
  }
}

void Mixer::applyModuleTempo() {
  if (!m_modulePlaying) {
    return;
  }
  double factor = m_moduleTempoFactor;
  if (m_tempoOverride > 0) {
    m_overrideTiming = moduleTiming();
    const auto [speed, bpm] = m_overrideTiming;
    if (speed > 0 && bpm > 0) {
      factor *= AMOS_TEMPO_PER_BPM * bpm / (speed * m_tempoOverride);
    }
  }
  xmp_set_tempo_factor(m_module->player, factor);
}

Mixer::RowPosition Mixer::modulePosition() const {
  xmp_frame_info info;
  xmp_get_frame_info(m_module->player, &info);
  return {info.pattern, info.row};
}

Mixer::ModuleTiming Mixer::moduleTiming() const {
  xmp_frame_info info;
  xmp_get_frame_info(m_module->player, &info);
  return {info.speed, info.bpm};
}

bool Mixer::playModule(std::size_t samples) {
  const std::size_t step = m_tempoOverride == 0 ? samples : TRACKED_SAMPLES;
  for (std::size_t done = 0; done < samples; done += step) {
    const std::size_t chunk = std::min(step, samples - done);
    if (xmp_play_buffer(m_module->player, m_musicBuffer.data() + done,
                        static_cast<int>(chunk * sizeof(int16_t)),
                        m_moduleLoops) < 0) {
      std::fill(m_musicBuffer.begin() + static_cast<std::ptrdiff_t>(done),
                m_musicBuffer.end(), 0);
      return false;
    }
    if (hasModuleEnded()) {
      return false;
    }
    followModuleTempo();
  }
  return true;
}

bool Mixer::hasModuleEnded() const {
  if (m_moduleLoops == 0) {
    return false;
  }
  xmp_frame_info info;
  xmp_get_frame_info(m_module->player, &info);
  return info.loop_count >= m_moduleLoops;
}

void Mixer::followModuleTempo() {
  if (m_tempoOverride == 0) {
    return;
  }
  const RowPosition position = modulePosition();
  const bool loopedBack =
      m_lastPosition != RowPosition{-1, -1} && position < m_lastPosition;
  if (!loopedBack && position != m_overridePosition &&
      m_tempoRows.count(position) > 0) {
    m_tempoOverride = 0;
    applyModuleTempo();
    m_overridePosition = position;
    m_lastPosition = position;
    return;
  }

  m_overridePosition = position;
  m_lastPosition = position;
  if (moduleTiming() != m_overrideTiming) {
    applyModuleTempo();
  }
}

bool Mixer::isSounding(const Playing &playing) const {
  return std::any_of(m_voices.begin(), m_voices.end(), [&](const Voice &voice) {
    return voice.sound == playing.sound && voice.frequency == playing.frequency;
  });
}

int Mixer::nextSample(Voice &voice) {
  const std::vector<int8_t> &frames = voice.sound->frames;
  const int sample =
      frames[static_cast<std::size_t>(voice.position >> FRACTION_BITS)] *
      BYTE_SCALE;
  voice.position += voice.step;
  const uint64_t end = static_cast<uint64_t>(frames.size()) << FRACTION_BITS;
  if (voice.position >= end) {
    if (voice.loop) {
      voice.position %= end;
    } else {
      voice = Voice{};
    }
  }
  return sample;
}

void Mixer::filter(int16_t *stereo, int frames) {
  const std::size_t samples = static_cast<std::size_t>(frames) * STEREO;
  for (std::size_t i = 0; i < samples; ++i) {
    std::array<double, 4> &history = m_filterHistory[i % STEREO];
    const double input = stereo[i];
    const double output = m_lowPass.b0 * input + m_lowPass.b1 * history[0] +
                          m_lowPass.b2 * history[1] -
                          m_lowPass.a1 * history[2] - m_lowPass.a2 * history[3];
    history = {input, history[0], output, history[2]};
    if (m_filterOn) {
      stereo[i] = clampSample(std::lround(output));
    }
  }
}

} // namespace openfranko::src::systems::audio
