#include "levelScript.h"
#include "../../helpers/helpers.h"
#include <cstdio>
#include <stdexcept>
#include <string>

namespace openfranko::lib::converter::levelScript {

namespace {

EnemySlot parseSlot(const std::vector<uint8_t> &data,
                    const helpers::BigEndianReader &reader, size_t off) {
  EnemySlot slot;
  slot.spriteSetId = data.at(off);

  if (slot.spriteSetId == consts::EMPTY_SLOT) {
    return slot;
  }

  slot.occupied = true;
  slot.kind = data.at(off + 1);
  slot.spawnX = reader.readInt16(off + 2);
  slot.spawnY = data.at(off + 4);
  slot.unused = data.at(off + 5);
  slot.energy = data.at(off + 6);
  slot.aggression = data.at(off + 7);
  return slot;
}

std::string escapeJson(const std::string &text) {
  std::string out;
  for (const char c : text) {
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (static_cast<unsigned char>(c) < 0x20) {
      char buf[7];
      snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
      out += buf;
    } else {
      out += c;
    }
  }
  return out;
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

} // anonymous namespace

Level parse(const std::vector<uint8_t> &decompressedData) {
  if (decompressedData.size() < consts::HEADER_SIZE) {
    throw std::runtime_error("Level script too small to contain a header");
  }

  const size_t body = decompressedData.size() - consts::HEADER_SIZE;
  if (body % consts::WAVE_SIZE != 0) {
    throw std::runtime_error("Level script is not a whole number of waves");
  }

  helpers::BigEndianReader reader(decompressedData);

  Level level;
  level.lengthInColumns = reader.readUint16(0);
  level.waves.reserve(body / consts::WAVE_SIZE);

  for (size_t off = consts::HEADER_SIZE; off < decompressedData.size();
       off += consts::WAVE_SIZE) {
    Wave wave;
    wave.triggerColumn = reader.readUint16(off);

    for (size_t i = 0; i < consts::SLOTS_PER_WAVE; ++i) {
      wave.slots[i] =
          parseSlot(decompressedData, reader, off + 2 + i * consts::SLOT_SIZE);
    }
    level.waves.push_back(wave);
  }
  return level;
}

std::vector<uint8_t> toJson(const Level &level, const std::string &fileId) {
  std::string out;
  out += "{\n";
  out += "  \"fileId\": \"" + escapeJson(fileId) + "\",\n";
  out += "  \"lengthInColumns\": " + std::to_string(level.lengthInColumns) +
         ",\n";
  out += "  \"waveCount\": " + std::to_string(level.waves.size()) + ",\n";
  out += "  \"waves\": [\n";

  for (size_t w = 0; w < level.waves.size(); ++w) {
    const Wave &wave = level.waves[w];
    out += "    {\n";
    out +=
        "      \"triggerColumn\": " + std::to_string(wave.triggerColumn) + ",\n";
    out += "      \"slots\": [\n";

    for (size_t i = 0; i < consts::SLOTS_PER_WAVE; ++i) {
      out += "        ";
      appendSlot(out, wave.slots[i]);
      out += (i + 1 < consts::SLOTS_PER_WAVE) ? ",\n" : "\n";
    }

    out += "      ]\n";
    out += (w + 1 < level.waves.size()) ? "    },\n" : "    }\n";
  }

  out += "  ]\n";
  out += "}\n";
  return std::vector<uint8_t>(out.begin(), out.end());
}

} // namespace openfranko::lib::converter::levelScript
