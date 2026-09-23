#ifndef LEVELSCRIPT_H_
#define LEVELSCRIPT_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace levelScript {

namespace consts {

inline constexpr uint8_t EMPTY_SLOT = 0xFF;
inline constexpr size_t SLOTS_PER_WAVE = 3;
inline constexpr size_t SLOT_SIZE = 8;
inline constexpr size_t WAVE_SIZE = 2 + SLOTS_PER_WAVE * SLOT_SIZE;
inline constexpr size_t HEADER_SIZE = 2;

} // namespace consts

namespace enemyKinds {

inline constexpr uint8_t BALD = 0;
inline constexpr uint8_t FROG = 1;
inline constexpr uint8_t KID = 2;

inline constexpr const char *name(uint8_t kind) {
  switch (kind) {
  case BALD:
    return "bald";
  case FROG:
    return "frog";
  case KID:
    return "kid";
  default:
    return "unknown";
  }
}

} // namespace enemyKinds

struct EnemySlot {
  bool occupied = false;
  uint8_t spriteSetId = consts::EMPTY_SLOT;
  uint8_t kind = 0;
  int16_t spawnX = 0;
  uint8_t spawnY = 0;
  uint8_t unused = 0;
  uint8_t energy = 0;
  uint8_t aggression = 0;
};

struct Wave {
  uint16_t triggerColumn = 0;
  std::array<EnemySlot, consts::SLOTS_PER_WAVE> slots;
};

struct Level {
  uint16_t lengthInColumns = 0;
  std::vector<Wave> waves;
};

Level parse(const std::vector<uint8_t> &decompressedData);

std::vector<uint8_t> toJson(const Level &level, const std::string &fileId);

} // namespace levelScript
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // LEVELSCRIPT_H_
