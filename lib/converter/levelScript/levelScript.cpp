#include "levelScript.h"
#include "../../binary/binary.h"
#include "../../json/json.h"

#include <stdexcept>
#include <string>

namespace openfranko::lib::converter::levelScript {

namespace {

constexpr size_t SLOT_KIND_OFFSET = 1;
constexpr size_t SLOT_SPAWN_X_OFFSET = 2;
constexpr size_t SLOT_SPAWN_Y_OFFSET = 4;
constexpr size_t SLOT_UNUSED_OFFSET = 5;
constexpr size_t SLOT_ENERGY_OFFSET = 6;
constexpr size_t SLOT_AGGRESSION_OFFSET = 7;

EnemySlot parseSlot(const std::vector<uint8_t> &data,
                    const binary::BigEndianReader &reader, size_t off) {
  EnemySlot slot;
  slot.spriteSetId = data.at(off);

  if (slot.spriteSetId == consts::EMPTY_SLOT) {
    return slot;
  }

  slot.occupied = true;
  slot.kind = data.at(off + SLOT_KIND_OFFSET);
  slot.spawnX = reader.readInt16(off + SLOT_SPAWN_X_OFFSET);
  slot.spawnY = data.at(off + SLOT_SPAWN_Y_OFFSET);
  slot.unused = data.at(off + SLOT_UNUSED_OFFSET);
  slot.energy = data.at(off + SLOT_ENERGY_OFFSET);
  slot.aggression = data.at(off + SLOT_AGGRESSION_OFFSET);
  return slot;
}

void appendSlot(std::string &out, const EnemySlot &slot) {
  if (!slot.occupied) {
    out += "null";
    return;
  }

  out += "{ \"spriteSetId\": " + std::to_string(slot.spriteSetId);
  out += ", \"kind\": " + std::to_string(slot.kind);
  out += ", \"kindName\": \"" + std::string(enemyKinds::name(slot.kind)) + "\"";
  out += ", \"spawnX\": " + std::to_string(slot.spawnX);
  out += ", \"spawnY\": " + std::to_string(slot.spawnY);
  out += ", \"unused\": " + std::to_string(slot.unused);
  out += ", \"energy\": " + std::to_string(slot.energy);
  out += ", \"aggression\": " + std::to_string(slot.aggression);
  out += " }";
}

} // namespace

Level parse(const std::vector<uint8_t> &data) {
  if (data.size() < consts::HEADER_SIZE) {
    throw std::runtime_error("Level script too small to contain a header");
  }

  const size_t body = data.size() - consts::HEADER_SIZE;
  if (body % consts::WAVE_SIZE != 0) {
    throw std::runtime_error("Level script is not a whole number of waves");
  }

  binary::BigEndianReader reader(data);

  Level level;
  level.lengthInColumns = reader.readUint16(0);
  level.waves.reserve(body / consts::WAVE_SIZE);

  for (size_t off = consts::HEADER_SIZE; off < data.size();
       off += consts::WAVE_SIZE) {
    Wave wave;
    wave.triggerColumn = reader.readUint16(off);

    for (size_t i = 0; i < consts::SLOTS_PER_WAVE; i++) {
      wave.slots[i] =
          parseSlot(data, reader,
                    off + consts::TRIGGER_COLUMN_SIZE + i * consts::SLOT_SIZE);
    }
    level.waves.push_back(wave);
  }
  return level;
}

std::vector<uint8_t> toJson(const Level &level, const std::string &fileId) {
  std::string out;
  out += "{\n";
  out += "  \"fileId\": \"" + json::escape(fileId) + "\",\n";
  out +=
      "  \"lengthInColumns\": " + std::to_string(level.lengthInColumns) + ",\n";
  out += "  \"waveCount\": " + std::to_string(level.waves.size()) + ",\n";
  out += "  \"waves\": [\n";

  for (size_t i = 0; i < level.waves.size(); i++) {
    const Wave &wave = level.waves[i];
    out += "    {\n";
    out += "      \"triggerColumn\": " + std::to_string(wave.triggerColumn) +
           ",\n";
    out += "      \"slots\": [\n";

    for (size_t j = 0; j < consts::SLOTS_PER_WAVE; j++) {
      out += "        ";
      appendSlot(out, wave.slots[j]);
      out += (j + 1 < consts::SLOTS_PER_WAVE) ? ",\n" : "\n";
    }

    out += "      ]\n";
    out += (i + 1 < level.waves.size()) ? "    },\n" : "    }\n";
  }

  out += "  ]\n";
  out += "}\n";
  return std::vector<uint8_t>(out.begin(), out.end());
}

} // namespace openfranko::lib::converter::levelScript
