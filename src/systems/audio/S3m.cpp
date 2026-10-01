#include "audio/S3m.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>

namespace openfranko::src::systems::audio {
namespace {

constexpr std::size_t ORDER_COUNT = 0x20;
constexpr std::size_t INSTRUMENT_COUNT = 0x22;
constexpr std::size_t PATTERN_COUNT = 0x24;
constexpr std::size_t FORMAT_INFO = 0x2A;
constexpr std::size_t SIGNATURE = 0x2C;
constexpr std::size_t INITIAL_SPEED = 0x31;
constexpr std::size_t INITIAL_TEMPO = 0x32;
constexpr std::size_t MASTER_VOLUME = 0x33;
constexpr std::size_t DEFAULT_PAN = 0x35;
constexpr std::size_t CHANNEL_SETTINGS = 0x40;
constexpr std::size_t ORDERS = 0x60;
constexpr std::size_t PARAGRAPH = 16;
constexpr std::size_t SAMPLE_HEADER = 0x50;
constexpr uint8_t SAMPLE_TYPE = 1;
constexpr uint8_t LOOP_FLAG = 1;
constexpr uint8_t STEREO_FLAG = 0x80;
constexpr uint8_t PAN_TABLE = 0xFC;
constexpr uint8_t PAN_SET = 0x20;
constexpr uint8_t RIGHT_CHANNELS = 8;
constexpr uint8_t ADLIB_CHANNELS = 16;
constexpr int LEFT_PAN = 0x30;
constexpr int RIGHT_PAN = 0xC0;
constexpr int CENTER_PAN = 0x80;
constexpr uint8_t NOTE_FOLLOWS = 0x20;
constexpr uint8_t VOLUME_FOLLOWS = 0x40;
constexpr uint8_t COMMAND_FOLLOWS = 0x80;
constexpr uint8_t CHANNEL_MASK = 0x1F;
constexpr uint8_t EMPTY_NOTE = 255;
constexpr uint8_t NOTE_OFF = 254;
constexpr uint8_t SET_SPEED = 1;
constexpr uint8_t SET_TEMPO = 20;
constexpr int SIGNED_FORMAT = 1;
constexpr double C4_RATE = 8363.0;
constexpr double FINETUNE_STEPS = 1536.0;
constexpr int FINETUNE_PER_NOTE = 128;

class Reader {
public:
  explicit Reader(const std::vector<uint8_t> &data) : m_data(data) {}

  uint8_t byte(std::size_t at) const {
    return at < m_data.size() ? m_data[at] : 0;
  }

  uint32_t word(std::size_t at) const { return byte(at) | byte(at + 1) << 8; }

  uint32_t longWord(std::size_t at) const {
    return word(at) | word(at + 2) << 16;
  }

  std::size_t size() const { return m_data.size(); }

private:
  const std::vector<uint8_t> &m_data;
};

void readSample(const Reader &file, std::size_t header, int format,
                S3mSample &sample) {
  if (file.byte(header) != SAMPLE_TYPE) {
    return;
  }
  const std::size_t memory =
      (static_cast<std::size_t>(file.byte(header + 13)) << 16 |
       file.word(header + 14)) *
      PARAGRAPH;
  const uint32_t length = file.longWord(header + 16);
  const uint32_t loopStart = file.longWord(header + 20);
  const uint32_t loopEnd = file.longWord(header + 24);
  sample.volume = std::min<int>(file.byte(header + 28), 64);
  const uint8_t flags = file.byte(header + 31);
  const uint32_t c2spd = file.word(header + 32);
  const std::size_t available =
      memory < file.size() ? std::min<std::size_t>(length, file.size() - memory)
                           : 0;
  sample.data.resize(available);
  for (std::size_t at = 0; at < available; ++at) {
    const uint8_t value = file.byte(memory + at);
    sample.data[at] = static_cast<int8_t>(
        format == SIGNED_FORMAT ? value : static_cast<uint8_t>(value - 128));
  }
  const uint32_t size = static_cast<uint32_t>(available);
  sample.loopEnd = std::min(loopEnd, size);
  sample.loopStart = std::min(loopStart, sample.loopEnd);
  sample.looped = (flags & LOOP_FLAG) != 0 && sample.loopEnd > sample.loopStart;
  if (c2spd > 0) {
    const int cents = static_cast<int>(
        FINETUNE_STEPS * std::log(static_cast<double>(c2spd) / C4_RATE) /
        std::log(2.0));
    sample.transpose = cents / FINETUNE_PER_NOTE;
    sample.finetune = cents % FINETUNE_PER_NOTE;
  }
}

void readPattern(const Reader &file, std::size_t start, int index,
                 S3mModule &module) {
  S3mModule::Pattern &pattern =
      module.patterns[static_cast<std::size_t>(index)];
  if (start == 0) {
    return;
  }
  const std::size_t end = std::min(file.size(), start + file.word(start));
  std::size_t at = start + 2;
  int row = 0;
  while (row < S3mModule::ROWS && at < end) {
    const uint8_t what = file.byte(at++);
    if (what == 0) {
      ++row;
      continue;
    }
    const int channel = what & CHANNEL_MASK;
    S3mEvent ignored;
    S3mEvent &event = channel < S3mModule::CHANNELS
                          ? pattern[static_cast<std::size_t>(row)]
                                   [static_cast<std::size_t>(channel)]
                          : ignored;
    if (what & NOTE_FOLLOWS) {
      const uint8_t note = file.byte(at);
      if (note == NOTE_OFF) {
        event.note = S3mModule::KEY_OFF;
      } else if (note != EMPTY_NOTE) {
        event.note = static_cast<uint8_t>(13 + 12 * (note >> 4) + (note & 15));
      }
      event.instrument = file.byte(at + 1);
      at += 2;
    }
    if (what & VOLUME_FOLLOWS) {
      event.volume = static_cast<uint8_t>(file.byte(at) + 1);
      ++at;
    }
    if (what & COMMAND_FOLLOWS) {
      event.command = file.byte(at);
      event.parameter = file.byte(at + 1);
      if (event.command == SET_SPEED || event.command == SET_TEMPO) {
        module.tempoRows.insert({index, row});
      }
      at += 2;
    }
  }
}

} // namespace

bool parseS3m(const std::vector<uint8_t> &data, S3mModule &module) {
  const Reader file(data);
  if (data.size() < ORDERS ||
      std::memcmp(data.data() + SIGNATURE, "SCRM", 4) != 0) {
    return false;
  }
  module = S3mModule{};
  const std::size_t orderCount = file.word(ORDER_COUNT);
  const std::size_t instrumentCount = file.word(INSTRUMENT_COUNT);
  const std::size_t patternCount = file.word(PATTERN_COUNT);
  const int format = static_cast<int>(file.word(FORMAT_INFO));
  module.speed = file.byte(INITIAL_SPEED) ? file.byte(INITIAL_SPEED) : 6;
  module.tempo =
      file.byte(INITIAL_TEMPO) >= 0x20 ? file.byte(INITIAL_TEMPO) : 125;
  const bool stereo = (file.byte(MASTER_VOLUME) & STEREO_FLAG) != 0;
  for (int channel = 0; channel < S3mModule::CHANNELS; ++channel) {
    const uint8_t setting =
        file.byte(CHANNEL_SETTINGS + static_cast<std::size_t>(channel)) & 0x7F;
    int pan = CENTER_PAN;
    if (stereo && setting < ADLIB_CHANNELS) {
      pan = setting < RIGHT_CHANNELS ? LEFT_PAN : RIGHT_PAN;
    }
    module.pans[static_cast<std::size_t>(channel)] = pan;
  }
  for (std::size_t order = 0; order < orderCount; ++order) {
    module.orders.push_back(file.byte(ORDERS + order));
  }
  const std::size_t instruments = ORDERS + orderCount;
  const std::size_t patterns = instruments + 2 * instrumentCount;
  if (file.byte(DEFAULT_PAN) == PAN_TABLE) {
    const std::size_t pans = patterns + 2 * patternCount;
    for (int channel = 0; channel < S3mModule::CHANNELS; ++channel) {
      const uint8_t value = file.byte(pans + static_cast<std::size_t>(channel));
      if (value & PAN_SET) {
        module.pans[static_cast<std::size_t>(channel)] = (value << 4) & 0xFF;
      }
    }
  }
  module.samples.resize(instrumentCount);
  for (std::size_t index = 0; index < instrumentCount; ++index) {
    const std::size_t header = file.word(instruments + 2 * index) * PARAGRAPH;
    if (header + SAMPLE_HEADER <= data.size()) {
      readSample(file, header, format, module.samples[index]);
    }
  }
  module.patterns.resize(patternCount);
  for (std::size_t index = 0; index < patternCount; ++index) {
    readPattern(file, file.word(patterns + 2 * index) * PARAGRAPH,
                static_cast<int>(index), module);
  }
  return true;
}

} // namespace openfranko::src::systems::audio
