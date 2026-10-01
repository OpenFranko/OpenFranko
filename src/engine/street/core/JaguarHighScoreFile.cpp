#include "HighScoreStorage.h"
#include "HighScoreTable.h"

#include "../../../systems/jaguar/Eeprom.h"

namespace openfranko::src::engine::street::core {

namespace eeprom = systems::jaguar::eeprom;

std::optional<HighScoreTable> readHighScoreFile(const std::string &) {
  eeprom::Bank bank{};
  if (!eeprom::readBank(bank)) {
    return std::nullopt;
  }
  StorageWords words{};
  for (std::size_t word = 0; word < STORAGE_WORDS; ++word) {
    words[word] = bank[word];
  }
  return unpackHighScores(words);
}

bool writeHighScoreFile(const HighScoreTable &table, const std::string &) {
  const StorageWords words = packHighScores(table);
  eeprom::Bank bank{};
  for (std::size_t word = 0; word < STORAGE_WORDS; ++word) {
    bank[word] = words[word];
  }
  eeprom::queueBank(bank);
  return true;
}

} // namespace openfranko::src::engine::street::core
