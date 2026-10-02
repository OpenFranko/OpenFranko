#include "LevelScript.h"

#include <stdexcept>
#include <utility>

namespace openfranko::src::engine::street::core {
namespace {

struct SlotField {
  std::string_view name;
  int EnemySlot::*field;
};

constexpr SlotField SLOT_FIELDS[] = {{"spriteSetId", &EnemySlot::spriteSet},
                                     {"kind", &EnemySlot::type},
                                     {"spawnX", &EnemySlot::x},
                                     {"spawnY", &EnemySlot::y},
                                     {"energy", &EnemySlot::energy},
                                     {"aggression", &EnemySlot::aggression}};

constexpr std::size_t FIELD_COUNT =
    sizeof(SLOT_FIELDS) / sizeof(SLOT_FIELDS[0]);

} // namespace

LevelScript LevelScript::fromJson(const std::string &json) {
  LevelScript script;
  LevelScriptReader reader(json);
  while (!reader.step(script)) {
  }
  return script;
}

LevelScriptReader::LevelScriptReader(std::string json)
    : m_json(std::move(json)) {}

bool LevelScriptReader::step(LevelScript &script) {
  if (!m_opened) {
    m_json.openObject();
    m_opened = true;
  }
  if (m_inWaves) {
    if (m_json.nextItem()) {
      readWave(script);
      return false;
    }
    m_inWaves = false;
  }
  std::string key;
  while (m_json.nextMember(key)) {
    if (key == "lengthInColumns" && !m_hasLength) {
      script.length = m_json.integer();
      m_hasLength = true;
    } else if (key == "waves" && !m_hasWaves) {
      m_json.openArray();
      m_hasWaves = true;
      m_inWaves = true;
      return false;
    } else {
      m_json.skip();
    }
  }
  m_json.finish();
  if (!m_hasLength) {
    missingMember("lengthInColumns");
  }
  if (!m_hasWaves) {
    missingMember("waves");
  }
  return true;
}

void LevelScriptReader::readWave(LevelScript &script) {
  Wave wave;
  bool hasTrigger = false;
  bool hasSlots = false;
  m_json.openObject();
  std::string key;
  while (m_json.nextMember(key)) {
    if (key == "triggerColumn" && !hasTrigger) {
      wave.trigger = m_json.integer();
      hasTrigger = true;
    } else if (key == "slots" && !hasSlots) {
      readSlots(wave);
      hasSlots = true;
    } else {
      m_json.skip();
    }
  }
  if (!hasTrigger) {
    missingMember("triggerColumn");
  }
  if (!hasSlots) {
    missingMember("slots");
  }
  script.waves.push_back(wave);
}

void LevelScriptReader::readSlots(Wave &wave) {
  std::size_t count = 0;
  m_json.openArray();
  while (m_json.nextItem()) {
    if (count < wave.slots.size()) {
      readSlot(wave.slots[count]);
    } else {
      m_json.skip();
    }
    ++count;
  }
  if (count != wave.slots.size()) {
    throw std::invalid_argument("Level script: a wave needs three slots");
  }
}

void LevelScriptReader::readSlot(EnemySlot &slot) {
  if (m_json.skipNull()) {
    return;
  }
  bool seen[FIELD_COUNT] = {};
  m_json.openObject();
  std::string key;
  while (m_json.nextMember(key)) {
    std::size_t field = 0;
    while (field < FIELD_COUNT &&
           (seen[field] || key != SLOT_FIELDS[field].name)) {
      ++field;
    }
    if (field == FIELD_COUNT) {
      m_json.skip();
      continue;
    }
    slot.*SLOT_FIELDS[field].field = m_json.integer();
    seen[field] = true;
  }
  for (std::size_t field = 0; field < FIELD_COUNT; ++field) {
    if (!seen[field]) {
      missingMember(SLOT_FIELDS[field].name);
    }
  }
}

} // namespace openfranko::src::engine::street::core
