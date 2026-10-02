#ifndef SYSTEMS_AUDIO_S3M_H_
#define SYSTEMS_AUDIO_S3M_H_

#include "Wave.h"

#include <array>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace audio {

struct S3mSample {
  std::vector<int8_t> data;
  uint32_t loopStart = 0;
  uint32_t loopEnd = 0;
  bool looped = false;
  int volume = 0;
  int transpose = 0;
  int finetune = 0;
};

struct S3mEvent {
  uint8_t note = 0;
  uint8_t instrument = 0;
  uint8_t volume = 0;
  uint8_t command = 0;
  uint8_t parameter = 0;
};

struct S3mModule {
  static constexpr int CHANNELS = 4;
  static constexpr int ROWS = 64;
  static constexpr uint8_t KEY_OFF = 0xFF;
  static constexpr uint8_t SKIP_ORDER = 0xFE;
  static constexpr uint8_t END_ORDER = 0xFF;

  using Row = std::array<S3mEvent, CHANNELS>;
  using Pattern = std::array<Row, ROWS>;

  int speed = 6;
  int tempo = 125;
  std::vector<uint8_t> orders;
  std::vector<Pattern> patterns;
  std::vector<S3mSample> samples;
  std::array<int, CHANNELS> pans{};
  std::set<std::pair<int, int>> tempoRows;
};

bool parseS3m(const std::vector<uint8_t> &data, S3mModule &module,
              SignFlip flip = nullptr);

} // namespace audio
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIO_S3M_H_
