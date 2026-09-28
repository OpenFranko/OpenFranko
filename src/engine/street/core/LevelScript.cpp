#include "LevelScript.h"

#include "Json.h"

#include <stdexcept>

namespace openfranko::src::engine::street {

LevelScript LevelScript::fromJson(const std::string &json) {
  const JsonValue root = parseJson(json);
  LevelScript script;
  script.length = root.member("lengthInColumns").integer();
  for (const JsonValue &record : root.member("waves").array()) {
    Wave wave;
    wave.trigger = record.member("triggerColumn").integer();
    const auto &slots = record.member("slots").array();
    if (slots.size() != wave.slots.size()) {
      throw std::invalid_argument("Level script: a wave needs three slots");
    }
    for (std::size_t i = 0; i < slots.size(); ++i) {
      if (slots[i].kind == JsonValue::Kind::Null) {
        continue;
      }
      EnemySlot &slot = wave.slots[i];
      slot.spriteSet = slots[i].member("spriteSetId").integer();
      slot.type = slots[i].member("kind").integer();
      slot.x = slots[i].member("spawnX").integer();
      slot.y = slots[i].member("spawnY").integer();
      slot.energy = slots[i].member("energy").integer();
      slot.aggression = slots[i].member("aggression").integer();
    }
    script.waves.push_back(wave);
  }
  return script;
}

} // namespace openfranko::src::engine::street
