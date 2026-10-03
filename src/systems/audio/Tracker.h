#ifndef SYSTEMS_AUDIO_TRACKER_H_
#define SYSTEMS_AUDIO_TRACKER_H_

#include "audio/S3m.h"

#include <array>
#include <cstdint>
#include <utility>

namespace openfranko {
namespace src {
namespace systems {
namespace audio {

struct TrackerVoice {
  int sample = -1;
  bool trigger = false;
  bool active = false;
  uint32_t period = 0;
  int gainLeft = 0;
  int gainRight = 0;
};

struct TrackerTick {
  std::array<TrackerVoice, S3mModule::CHANNELS> voices;
  int pattern = 0;
  int row = 0;
  int speed = 0;
  int bpm = 0;
  bool restarted = false;
};

class Tracker {
public:
  static constexpr int PERIOD_ONE = 256;
  static constexpr uint32_t C4_PERIOD = 428;
  static constexpr uint32_t C4_RATE = 8363;

  explicit Tracker(const S3mModule &module);

  void restart();
  const TrackerTick &advance();
  int loops() const;
  bool isEmpty() const;

  static uint32_t notePeriod(int note, int finetune);
  static uint32_t transposed(uint32_t period, int semitones);

private:
  struct Channel {
    int sample = -1;
    int volume = 0;
    uint32_t period = 0;
    int32_t slide = 0;
    int slideMemory = 0;
    int arpeggioMemory = 0;
    std::array<int, 3> arpeggio{};
    int arpeggioCount = 0;
    bool arpeggioOn = false;
    bool pitchBend = false;
    bool sounding = false;
    bool trigger = false;
  };

  void readRow();
  void readEvent(Channel &channel, const S3mEvent &event);
  void applyEffect(Channel &channel, const S3mEvent &event);
  void updateFrequency(Channel &channel);
  void advanceRow();
  int nextOrder(int order);
  void output();

  const S3mModule &m_module;
  std::array<Channel, S3mModule::CHANNELS> m_channels;
  TrackerTick m_tick;
  int m_order = 0;
  int m_row = 0;
  int m_frame = 0;
  int m_speed = 6;
  int m_bpm = 125;
  int m_loops = 0;
  bool m_break = false;
  int m_breakRow = 0;
  int m_jumpOrder = -1;
  bool m_restarted = false;
  bool m_empty = false;
};

} // namespace audio
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIO_TRACKER_H_
