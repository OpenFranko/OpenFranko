#include "HighScoreTable.h"

#include <algorithm>
#include <cstddef>
#include <fstream>

namespace openfranko::src::engine::street {
namespace {

constexpr const char *SEED_NAME = "XPSME TPGUXBSF ";
constexpr int SEED_SHIFT = 66;
constexpr const char *VERSION12_SEED_NAME = "NO NAMED HERO  ";
constexpr int VERSION12_SEED_SHIFT = 65;
constexpr int LETTER_BASE = 'A';
constexpr uint8_t BLANK = 0xFF;
constexpr int ROTATION = 6;

std::size_t offset(int row, int column) {
  return static_cast<std::size_t>(row * HighScoreTable::RECORD_SIZE + column);
}

uint32_t readLong(const HighScoreTable::Bytes &bytes, std::size_t at) {
  return static_cast<uint32_t>(bytes[at]) << 24 |
         static_cast<uint32_t>(bytes[at + 1]) << 16 |
         static_cast<uint32_t>(bytes[at + 2]) << 8 |
         static_cast<uint32_t>(bytes[at + 3]);
}

void writeLong(HighScoreTable::Bytes &bytes, std::size_t at, uint32_t value) {
  bytes[at] = static_cast<uint8_t>(value >> 24);
  bytes[at + 1] = static_cast<uint8_t>(value >> 16);
  bytes[at + 2] = static_cast<uint8_t>(value >> 8);
  bytes[at + 3] = static_cast<uint8_t>(value);
}

HighScoreTable::Bytes rotateLongs(HighScoreTable::Bytes bytes, bool left) {
  for (std::size_t at = 0; at < bytes.size(); at += 4) {
    const uint32_t value = readLong(bytes, at);
    writeLong(bytes, at,
              left ? (value << ROTATION | value >> (32 - ROTATION))
                   : (value >> ROTATION | value << (32 - ROTATION)));
  }
  return bytes;
}

} // namespace

HighScoreTable::HighScoreTable() : HighScoreTable(GameVersion::V10) {}

HighScoreTable::HighScoreTable(GameVersion version) {
  const bool version12 = version == GameVersion::V12;
  const char *seed = version12 ? VERSION12_SEED_NAME : SEED_NAME;
  const int shift = version12 ? VERSION12_SEED_SHIFT : SEED_SHIFT;
  for (int row = 0; row < ROWS; ++row) {
    for (int column = 0; column < NAME_LENGTH; ++column) {
      m_bytes[offset(row, column)] = static_cast<uint8_t>(seed[column] - shift);
    }
    m_bytes[offset(row, NAME_LENGTH)] = 0;
  }
}

HighScoreTable::HighScoreTable(const Bytes &bytes) : m_bytes(bytes) {}

HighScoreTable HighScoreTable::fromFile(const Bytes &file) {
  return HighScoreTable(rotateLongs(file, true));
}

HighScoreTable::Bytes HighScoreTable::toFile() const {
  return rotateLongs(m_bytes, false);
}

uint8_t HighScoreTable::letter(int row, int column) const {
  return m_bytes.at(offset(row, column));
}

int HighScoreTable::score(int row) const {
  return m_bytes.at(offset(row, NAME_LENGTH));
}

const HighScoreTable::Bytes &HighScoreTable::bytes() const { return m_bytes; }

int HighScoreTable::insert(int kills) {
  int slot = 0;
  while (slot < ROWS && kills < score(slot)) {
    ++slot;
  }
  for (int row = ROWS - 2; row >= slot; --row) {
    std::copy_n(m_bytes.begin() + offset(row, 0), RECORD_SIZE,
                m_bytes.begin() + offset(row + 1, 0));
  }
  if (slot < ROWS) {
    m_bytes[offset(slot, NAME_LENGTH)] = static_cast<uint8_t>(kills);
    std::fill_n(m_bytes.begin() + offset(slot, 0), NAME_LENGTH, BLANK);
  }
  return slot;
}

void HighScoreTable::setName(int row, const std::string &name) {
  for (int column = 0; column < NAME_LENGTH; ++column) {
    const std::size_t index = static_cast<std::size_t>(column);
    const char character = index < name.size() ? name[index] : ' ';
    m_bytes.at(offset(row, column)) =
        static_cast<uint8_t>(character - LETTER_BASE);
  }
}

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

} // namespace openfranko::src::engine::street
