#include "HighScoreTable.h"

#include <fstream>

namespace openfranko::src::engine::street::core {

std::optional<HighScoreTable> readHighScoreFile(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return std::nullopt;
  }
  HighScoreTable::Bytes bytes{};
  file.read(reinterpret_cast<char *>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  return HighScoreTable::fromFile(bytes);
}

bool writeHighScoreFile(const HighScoreTable &table, const std::string &path) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file) {
    return false;
  }
  const HighScoreTable::Bytes bytes = table.toFile();
  file.write(reinterpret_cast<const char *>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
  return static_cast<bool>(file);
}

} // namespace openfranko::src::engine::street::core
