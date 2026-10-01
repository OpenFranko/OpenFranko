#include "audio/Tracker.h"
#include "Multiply.h"

#include <algorithm>
#include <cmath>

namespace openfranko::src::systems::audio {
namespace {

using systems::multiplySigned16;
using systems::multiplyUnsigned16;

constexpr int SEMITONES = 12;
constexpr int FINE_STEPS = 128;
constexpr int OCTAVE_STEPS = SEMITONES * FINE_STEPS;
constexpr uint64_t PERIOD_BASE = 13696ull * Tracker::PERIOD_ONE;
constexpr int FRACTION = 30;
constexpr int HALF_BITS = 16;
constexpr double FRACTION_ONE = 1073741824.0;
constexpr uint32_t MIN_PERIOD = 1 * Tracker::PERIOD_ONE;
constexpr uint32_t MAX_PERIOD = 0xFFFF * Tracker::PERIOD_ONE;
constexpr int CENTER_PAN = 0x80;
constexpr int VOLUME_SCALE = 16;
constexpr int GAIN_SHIFT = 8;
constexpr int ARPEGGIO_STEPS = 3;
constexpr int MAX_VOLUME = 64;
constexpr int MAX_NOTE = 120;

constexpr uint8_t SET_SPEED = 1;
constexpr uint8_t JUMP = 2;
constexpr uint8_t BREAK = 3;
constexpr uint8_t SLIDE_DOWN = 5;
constexpr uint8_t SLIDE_UP = 6;
constexpr uint8_t ARPEGGIO = 10;
constexpr uint8_t SET_TEMPO = 20;
constexpr int MIN_TEMPO = 0x20;
constexpr int FINE_SLIDE = 0xF0;
constexpr int EXTRA_FINE_SLIDE = 0xE0;

struct Tables {
  std::array<uint32_t, SEMITONES> semitone{};
  std::array<uint32_t, FINE_STEPS> fine{};

  Tables() {
    for (int step = 0; step < SEMITONES; ++step) {
      semitone[static_cast<std::size_t>(step)] = static_cast<uint32_t>(
          std::floor(FRACTION_ONE * std::pow(2.0, -step / 12.0) + 0.5));
    }
    for (int step = 0; step < FINE_STEPS; ++step) {
      fine[static_cast<std::size_t>(step)] = static_cast<uint32_t>(
          std::floor(FRACTION_ONE * std::pow(2.0, -step / 1536.0) + 0.5));
    }
  }
};

const Tables &tables() {
  static const Tables instance;
  return instance;
}

uint32_t halfProduct(uint32_t left, uint32_t right) {
  return multiplyUnsigned16(static_cast<uint16_t>(left),
                            static_cast<uint16_t>(right));
}

uint64_t product(uint32_t left, uint32_t right) {
  const uint32_t leftHigh = left >> HALF_BITS;
  const uint32_t rightHigh = right >> HALF_BITS;
  return (static_cast<uint64_t>(halfProduct(leftHigh, rightHigh))
          << (2 * HALF_BITS)) +
         (static_cast<uint64_t>(halfProduct(leftHigh, right)) << HALF_BITS) +
         (static_cast<uint64_t>(halfProduct(left, rightHigh)) << HALF_BITS) +
         halfProduct(left, right);
}

uint32_t octavePeriod(int within) {
  static std::array<uint32_t, OCTAVE_STEPS> periods{};
  uint32_t &period = periods[static_cast<std::size_t>(within)];
  if (period == 0) {
    const Tables &table = tables();
    uint64_t value = PERIOD_BASE;
    value =
        value * table.semitone[static_cast<std::size_t>(within / FINE_STEPS)] >>
        FRACTION;
    value = value * table.fine[static_cast<std::size_t>(within % FINE_STEPS)] >>
            FRACTION;
    period = static_cast<uint32_t>(value);
  }
  return period;
}

int gain(int volume, int side) {
  return multiplySigned16(static_cast<int16_t>(VOLUME_SCALE * volume),
                          static_cast<int16_t>(side)) >>
         GAIN_SHIFT;
}

uint32_t clampPeriod(int64_t period) {
  return static_cast<uint32_t>(
      std::clamp<int64_t>(period, MIN_PERIOD, MAX_PERIOD));
}

} // namespace

Tracker::Tracker(const S3mModule &module) : m_module(module) { restart(); }

uint32_t Tracker::notePeriod(int note, int finetune) {
  int within = note * FINE_STEPS + finetune;
  int octave = 0;
  while (within < 0) {
    within += OCTAVE_STEPS;
    --octave;
  }
  while (within >= OCTAVE_STEPS) {
    within -= OCTAVE_STEPS;
    ++octave;
  }
  uint64_t period = octavePeriod(within);
  if (octave >= 0) {
    period >>= octave;
  } else {
    period <<= -octave;
  }
  return clampPeriod(static_cast<int64_t>(period));
}

uint32_t Tracker::transposed(uint32_t period, int semitones) {
  if (semitones <= 0) {
    return period;
  }
  int octaves = 0;
  while (semitones >= SEMITONES) {
    semitones -= SEMITONES;
    ++octaves;
  }
  const uint64_t scaled =
      product(period, tables().semitone[static_cast<std::size_t>(semitones)]) >>
      FRACTION;
  return clampPeriod(static_cast<int64_t>(scaled >> octaves));
}

void Tracker::restart() {
  m_channels.fill(Channel{});
  m_speed = m_module.speed;
  m_bpm = m_module.tempo;
  m_row = 0;
  m_frame = 0;
  m_loops = 0;
  m_break = false;
  m_jumpOrder = -1;
  m_restarted = false;
  m_order = 0;
  m_empty = true;
  for (std::size_t order = 0; order < m_module.orders.size(); ++order) {
    const uint8_t pattern = m_module.orders[order];
    if (pattern == S3mModule::END_ORDER) {
      break;
    }
    if (pattern != S3mModule::SKIP_ORDER &&
        pattern < m_module.patterns.size()) {
      m_order = static_cast<int>(order);
      m_empty = false;
      break;
    }
  }
}

int Tracker::loops() const { return m_loops; }

bool Tracker::isEmpty() const { return m_empty; }

const TrackerTick &Tracker::advance() {
  m_tick.restarted = m_restarted;
  m_restarted = false;
  if (m_empty) {
    m_tick.voices.fill(TrackerVoice{});
    m_tick.speed = m_speed;
    m_tick.bpm = m_bpm;
    return m_tick;
  }
  if (m_frame == 0) {
    readRow();
  }
  for (Channel &channel : m_channels) {
    updateFrequency(channel);
  }
  m_tick.pattern = m_module.orders[static_cast<std::size_t>(m_order)];
  m_tick.row = m_row;
  m_tick.speed = m_speed;
  m_tick.bpm = m_bpm;
  output();
  if (++m_frame >= m_speed) {
    m_frame = 0;
    advanceRow();
  }
  return m_tick;
}

void Tracker::readRow() {
  const int pattern = m_module.orders[static_cast<std::size_t>(m_order)];
  const S3mModule::Row &row =
      m_module.patterns[static_cast<std::size_t>(pattern)]
                       [static_cast<std::size_t>(m_row)];
  for (int index = 0; index < S3mModule::CHANNELS; ++index) {
    readEvent(m_channels[static_cast<std::size_t>(index)],
              row[static_cast<std::size_t>(index)]);
  }
}

void Tracker::readEvent(Channel &channel, const S3mEvent &event) {
  channel.pitchBend = false;
  channel.arpeggioOn = false;
  const bool validNote = event.note > 0 && event.note <= MAX_NOTE;
  int key = -1;
  if (validNote) {
    key = event.note - 1;
  }
  if (event.instrument > 0 &&
      event.instrument <= static_cast<int>(m_module.samples.size())) {
    channel.sample = event.instrument - 1;
    if (event.note != S3mModule::KEY_OFF) {
      channel.volume =
          m_module.samples[static_cast<std::size_t>(channel.sample)].volume;
    }
  }
  if (event.note == S3mModule::KEY_OFF) {
    channel.sounding = false;
  } else if (validNote && channel.sample >= 0) {
    const S3mSample &sample =
        m_module.samples[static_cast<std::size_t>(channel.sample)];
    channel.period = notePeriod(key + sample.transpose, sample.finetune);
    channel.sounding = !sample.data.empty();
    channel.trigger = channel.sounding;
  }
  if (event.volume > 0) {
    channel.volume = std::min(event.volume - 1, MAX_VOLUME);
  }
  applyEffect(channel, event);
}

void Tracker::applyEffect(Channel &channel, const S3mEvent &event) {
  const int parameter = event.parameter;
  switch (event.command) {
  case SET_SPEED:
    if (parameter > 0) {
      m_speed = parameter;
    }
    break;
  case JUMP:
    m_break = true;
    m_breakRow = 0;
    m_jumpOrder = parameter;
    break;
  case BREAK: {
    const int row = (parameter >> 4) * 10 + (parameter & 15);
    m_break = true;
    m_breakRow = row < S3mModule::ROWS ? row : 0;
    break;
  }
  case SLIDE_DOWN:
  case SLIDE_UP: {
    const int amount = parameter != 0 ? parameter : channel.slideMemory;
    channel.slideMemory = amount;
    const int sign = event.command == SLIDE_UP ? -1 : 1;
    if ((amount & 0xF0) == FINE_SLIDE) {
      channel.period = clampPeriod(static_cast<int64_t>(channel.period) +
                                   sign * (amount & 15) * PERIOD_ONE);
    } else if ((amount & 0xF0) == EXTRA_FINE_SLIDE) {
      channel.period = clampPeriod(static_cast<int64_t>(channel.period) +
                                   sign * (amount & 15) * PERIOD_ONE / 4);
    } else if (amount != 0) {
      channel.slide = sign * amount * PERIOD_ONE;
      channel.pitchBend = true;
    }
    break;
  }
  case ARPEGGIO: {
    const int value = parameter != 0 ? parameter : channel.arpeggioMemory;
    channel.arpeggioMemory = value;
    channel.arpeggio = {0, value >> 4, value & 15};
    channel.arpeggioOn = true;
    break;
  }
  case SET_TEMPO:
    if (parameter >= MIN_TEMPO) {
      m_bpm = parameter;
    }
    break;
  default:
    break;
  }
}

void Tracker::updateFrequency(Channel &channel) {
  if (m_frame != 0 && channel.pitchBend) {
    channel.period =
        clampPeriod(static_cast<int64_t>(channel.period) + channel.slide);
  }
}

void Tracker::output() {
  for (int index = 0; index < S3mModule::CHANNELS; ++index) {
    Channel &channel = m_channels[static_cast<std::size_t>(index)];
    TrackerVoice &voice = m_tick.voices[static_cast<std::size_t>(index)];
    int arpeggio = 0;
    if (channel.arpeggioOn) {
      arpeggio =
          channel.arpeggio[static_cast<std::size_t>(channel.arpeggioCount)];
      channel.arpeggioCount = channel.arpeggioCount + 1 < ARPEGGIO_STEPS
                                  ? channel.arpeggioCount + 1
                                  : 0;
    }
    voice.sample = channel.sample;
    voice.trigger = channel.trigger;
    voice.active = channel.sounding;
    voice.period = transposed(channel.period, arpeggio);
    const int volume = channel.sounding ? channel.volume : 0;
    const int pan = m_module.pans[static_cast<std::size_t>(index)] - CENTER_PAN;
    voice.gainLeft = gain(volume, CENTER_PAN - pan);
    voice.gainRight = gain(volume, CENTER_PAN + pan);
    channel.trigger = false;
  }
}

int Tracker::nextOrder(int order) {
  const int count = static_cast<int>(m_module.orders.size());
  for (int next = order + 1;; ++next) {
    if (next >= count || m_module.orders[static_cast<std::size_t>(next)] ==
                             S3mModule::END_ORDER) {
      ++m_loops;
      m_restarted = true;
      next = -1;
      continue;
    }
    const uint8_t pattern = m_module.orders[static_cast<std::size_t>(next)];
    if (pattern != S3mModule::SKIP_ORDER &&
        pattern < m_module.patterns.size()) {
      return next;
    }
  }
}

void Tracker::advanceRow() {
  if (m_break) {
    m_break = false;
    if (m_jumpOrder >= 0) {
      if (m_jumpOrder <= m_order) {
        ++m_loops;
        m_restarted = true;
      }
      m_order = nextOrder(m_jumpOrder - 1);
      m_jumpOrder = -1;
    } else {
      m_order = nextOrder(m_order);
    }
    m_row = m_breakRow;
    return;
  }
  if (++m_row >= S3mModule::ROWS) {
    m_row = 0;
    m_order = nextOrder(m_order);
  }
}

} // namespace openfranko::src::systems::audio
