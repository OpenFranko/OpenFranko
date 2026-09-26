#ifndef ENGINE_STREET_HIGHSCORETABLE_H_
#define ENGINE_STREET_HIGHSCORETABLE_H_

#include "../GameVersion.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

class HighScoreTable {
public:
  static constexpr int ROWS = 10;
  static constexpr int NAME_LENGTH = 15;
  static constexpr int RECORD_SIZE = NAME_LENGTH + 1;
  static constexpr int SIZE = ROWS * RECORD_SIZE;
  static constexpr int NO_SLOT = ROWS;
  static constexpr int LETTERS = 26;
  static constexpr const char *FILE_NAME = "h";

  using Bytes = std::array<uint8_t, SIZE>;

  HighScoreTable();
  explicit HighScoreTable(GameVersion version);

  static HighScoreTable fromFile(const Bytes &file);
  Bytes toFile() const;

  uint8_t letter(int row, int column) const;
  int score(int row) const;
  const Bytes &bytes() const;

  int insert(int kills);
  void setName(int row, const std::string &name);

private:
  explicit HighScoreTable(const Bytes &bytes);

  Bytes m_bytes{};
};

std::optional<HighScoreTable> readHighScoreFile(const std::string &path);
bool writeHighScoreFile(const HighScoreTable &table, const std::string &path);

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_HIGHSCORETABLE_H_
