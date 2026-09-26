#include "Mixer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <xmp.h>

namespace openfranko::src::systems {
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
    : rate(outputRate), module(std::make_unique<Module>()),
      musicVolume(DEFAULT_MUSIC_VOLUME) {
  if (!module->player) {
    throw std::runtime_error("Mixer error: no module player");
  }
  const double w0 = 2.0 * PI * LED_FILTER_HERTZ / rate;
  const double alpha = std::sin(w0) / (2.0 * BUTTERWORTH_Q);
  const double cosine = std::cos(w0);
  const double a0 = 1.0 + alpha;
  lowPass = {(1.0 - cosine) / 2.0 / a0, (1.0 - cosine) / a0,
             (1.0 - cosine) / 2.0 / a0, -2.0 * cosine / a0, (1.0 - alpha) / a0};
}

Mixer::~Mixer() {
  stopPlayer();
  if (moduleLoaded) {
    xmp_release_module(module->player);
  }
}

bool Mixer::loadModule(const std::vector<char> &data) {
  std::lock_guard<std::mutex> lock(mutex);
  stopPlayer();
  if (moduleLoaded) {
    xmp_release_module(module->player);
  }
  moduleLoaded = !data.empty() && xmp_load_module_from_memory(
                                      module->player, data.data(),
                                      static_cast<long>(data.size())) == 0;
  tempoRows = moduleLoaded ? s3mTempoRows(data) : std::set<RowPosition>{};
  return moduleLoaded;
}

void Mixer::releaseModule() {
  std::lock_guard<std::mutex> lock(mutex);
  stopPlayer();
  if (moduleLoaded) {
    xmp_release_module(module->player);
    moduleLoaded = false;
  }
  tempoRows.clear();
}

void Mixer::startModule() {
  std::lock_guard<std::mutex> lock(mutex);
  stopPlayer();
  if (moduleLoaded && xmp_start_player(module->player, rate, 0) == 0) {
    modulePlaying = true;
  }
}

void Mixer::stopModule() {
  std::lock_guard<std::mutex> lock(mutex);
  stopPlayer();
}

bool Mixer::isModulePlaying() const {
  std::lock_guard<std::mutex> lock(mutex);
  return modulePlaying;
}

void Mixer::setModuleTempo(double factor) {
  std::lock_guard<std::mutex> lock(mutex);
  moduleTempoFactor = factor;
  applyModuleTempo();
}

void Mixer::overrideModuleTempo(int tempo) {
  std::lock_guard<std::mutex> lock(mutex);
  if (!modulePlaying || tempo <= 0) {
    return;
  }
  tempoOverride = tempo;
  overridePosition = modulePosition();
  applyModuleTempo();
}

bool Mixer::isModuleTempoOverridden() const {
  std::lock_guard<std::mutex> lock(mutex);
  return tempoOverride > 0;
}

void Mixer::setMusicVolume(int volume) {
  std::lock_guard<std::mutex> lock(mutex);
  musicVolume = volume;
}

void Mixer::setFilter(bool on) {
  std::lock_guard<std::mutex> lock(mutex);
  filterOn = on;
}

void Mixer::play(const Sound &sound, int voiceMask, int frequency, bool loop) {
  std::lock_guard<std::mutex> lock(mutex);
  const int playRate = frequency > 0 ? frequency : sound.rate;
  for (int voice = 0; voice < VOICES; ++voice) {
    if ((voiceMask & (1 << voice)) == 0) {
      continue;
    }
    Voice &target = voices[static_cast<std::size_t>(voice)];
    target = Voice{};
    if (!sound.frames.empty() && playRate > 0) {
      target.sound = &sound;
      target.frequency = frequency;
      target.step = (static_cast<uint64_t>(playRate) << FRACTION_BITS) /
                    static_cast<uint64_t>(rate);
      target.loop = loop;
    }
  }
  if ((voiceMask & ALL_VOICES) == ALL_VOICES) {
    silencing = Playing{&sound, frequency};
  }
}

void Mixer::endLoops() {
  std::lock_guard<std::mutex> lock(mutex);
  for (Voice &voice : voices) {
    voice.loop = false;
  }
}

void Mixer::stop(const Sound &sound) {
  std::lock_guard<std::mutex> lock(mutex);
  for (Voice &voice : voices) {
    if (voice.sound == &sound) {
      voice = Voice{};
    }
  }
  if (silencing && silencing->sound == &sound) {
    silencing.reset();
  }
}

void Mixer::stopAll() {
  std::lock_guard<std::mutex> lock(mutex);
  voices.fill(Voice{});
}

void Mixer::update() {
  std::lock_guard<std::mutex> lock(mutex);
  if (silencing && !isSounding(*silencing)) {
    silencing.reset();
  }
}

bool Mixer::isPlaying(int voice) const {
  std::lock_guard<std::mutex> lock(mutex);
  return voices.at(static_cast<std::size_t>(voice)).sound != nullptr;
}

bool Mixer::isMusicSilenced() const {
  std::lock_guard<std::mutex> lock(mutex);
  return silencing.has_value();
}

void Mixer::render(int16_t *stereo, int frames) {
  std::lock_guard<std::mutex> lock(mutex);
  const std::size_t samples = static_cast<std::size_t>(frames) * STEREO;
  musicBuffer.assign(samples, 0);
  if (modulePlaying && !playModule(samples)) {
    std::fill(musicBuffer.begin(), musicBuffer.end(), 0);
  }

  const int musicLevel = silencing ? 0 : musicVolume;
  for (std::size_t frame = 0; frame < static_cast<std::size_t>(frames);
       ++frame) {
    int16_t *out = stereo + frame * STEREO;
    for (std::size_t channel = 0; channel < STEREO; ++channel) {
      out[channel] = clampSample(musicBuffer[frame * STEREO + channel] *
                                 musicLevel / MAX_VOLUME);
    }
    for (int voice = 0; voice < VOICES; ++voice) {
      Voice &playing = voices[static_cast<std::size_t>(voice)];
      if (!playing.sound) {
        continue;
      }
      const int sample = nextSample(playing) * SAMPLE_VOLUME / MAX_VOLUME /
                         VOICES_PER_SIDE;
      int16_t &side = out[isLeftVoice(voice) ? 0 : 1];
      side = clampSample(side + sample);
    }
  }
  filter(stereo, frames);
}

void Mixer::stopPlayer() {
  tempoOverride = 0;
  if (modulePlaying) {
    xmp_end_player(module->player);
    modulePlaying = false;
  }
}

void Mixer::applyModuleTempo() {
  if (!modulePlaying) {
    return;
  }
  double factor = moduleTempoFactor;
  if (tempoOverride > 0) {
    xmp_frame_info info;
    xmp_get_frame_info(module->player, &info);
    if (info.speed > 0 && info.bpm > 0) {
      factor *= AMOS_TEMPO_PER_BPM * info.bpm / (info.speed * tempoOverride);
    }
  }
  xmp_set_tempo_factor(module->player, factor);
}

Mixer::RowPosition Mixer::modulePosition() const {
  xmp_frame_info info;
  xmp_get_frame_info(module->player, &info);
  return {info.pattern, info.row};
}

bool Mixer::playModule(std::size_t samples) {
  if (tempoOverride == 0) {
    return xmp_play_buffer(module->player, musicBuffer.data(),
                           static_cast<int>(samples * sizeof(int16_t)), 0) >= 0;
  }
  for (std::size_t done = 0; done < samples; done += TRACKED_SAMPLES) {
    const std::size_t chunk = std::min(TRACKED_SAMPLES, samples - done);
    if (xmp_play_buffer(module->player, musicBuffer.data() + done,
                        static_cast<int>(chunk * sizeof(int16_t)), 0) < 0) {
      return false;
    }
    followModuleTempo();
  }
  return true;
}

void Mixer::followModuleTempo() {
  if (tempoOverride == 0) {
    return;
  }
  const RowPosition position = modulePosition();
  if (position == overridePosition) {
    return;
  }
  overridePosition = position;
  if (tempoRows.count(position) > 0) {
    tempoOverride = 0;
    applyModuleTempo();
  }
}

bool Mixer::isSounding(const Playing &playing) const {
  return std::any_of(voices.begin(), voices.end(), [&](const Voice &voice) {
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
    std::array<double, 4> &history = filterHistory[i % STEREO];
    const double input = stereo[i];
    const double output = lowPass.b0 * input + lowPass.b1 * history[0] +
                          lowPass.b2 * history[1] - lowPass.a1 * history[2] -
                          lowPass.a2 * history[3];
    history = {input, history[0], output, history[2]};
    if (filterOn) {
      stereo[i] = clampSample(std::lround(output));
    }
  }
}

} // namespace openfranko::src::systems
