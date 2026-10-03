#include "HighScoreStorage.h"

namespace openfranko::src::engine::street::core {
namespace {

constexpr uint8_t MAGIC = 0xF6;
constexpr int CODE_BITS = 6;
constexpr int SCORE_BITS = 8;
constexpr uint8_t TYPED_SPACE = 0xDF;
constexpr uint8_t SEED_SPACE = 0xDE;
constexpr uint8_t BLANK = 0xFF;
constexpr uint8_t TYPED_SPACE_CODE = HighScoreTable::LETTERS;
constexpr uint8_t SEED_SPACE_CODE = HighScoreTable::LETTERS + 1;
constexpr uint8_t BLANK_CODE = HighScoreTable::LETTERS + 2;
constexpr std::size_t STORAGE_BYTES = STORAGE_WORDS * 2;

uint8_t letterCode(uint8_t letter) {
  if (letter < HighScoreTable::LETTERS) {
    return letter;
  }
  if (letter == SEED_SPACE) {
    return SEED_SPACE_CODE;
  }
  if (letter == BLANK) {
    return BLANK_CODE;
  }
  return TYPED_SPACE_CODE;
}

uint8_t letterValue(uint8_t code) {
  if (code < HighScoreTable::LETTERS) {
    return code;
  }
  if (code == SEED_SPACE_CODE) {
    return SEED_SPACE;
  }
  if (code == BLANK_CODE) {
    return BLANK;
  }
  return TYPED_SPACE;
}

class BitWriter {
public:
  explicit BitWriter(std::array<uint8_t, STORAGE_BYTES> &bytes)
      : m_bytes(bytes) {}

  void put(unsigned value, int bits) {
    for (int bit = bits - 1; bit >= 0; --bit) {
      if ((value >> bit) & 1) {
        m_bytes[m_position / 8] |=
            static_cast<uint8_t>(0x80 >> (m_position % 8));
      }
      ++m_position;
    }
  }

private:
  std::array<uint8_t, STORAGE_BYTES> &m_bytes;
  std::size_t m_position = 8;
};

class BitReader {
public:
  explicit BitReader(const std::array<uint8_t, STORAGE_BYTES> &bytes)
      : m_bytes(bytes) {}

  unsigned get(int bits) {
    unsigned value = 0;
    for (int bit = 0; bit < bits; ++bit) {
      value =
          value << 1 | ((m_bytes[m_position / 8] >> (7 - m_position % 8)) & 1u);
      ++m_position;
    }
    return value;
  }

private:
  const std::array<uint8_t, STORAGE_BYTES> &m_bytes;
  std::size_t m_position = 8;
};

} // namespace

StorageWords packHighScores(const HighScoreTable &table) {
  std::array<uint8_t, STORAGE_BYTES> bytes{};
  bytes[0] = MAGIC;
  BitWriter writer(bytes);
  for (int row = 0; row < HighScoreTable::ROWS; ++row) {
    for (int column = 0; column < HighScoreTable::NAME_LENGTH; ++column) {
      writer.put(letterCode(table.letter(row, column)), CODE_BITS);
    }
    writer.put(static_cast<unsigned>(table.score(row)), SCORE_BITS);
  }
  StorageWords words{};
  for (std::size_t word = 0; word < STORAGE_WORDS; ++word) {
    words[word] =
        static_cast<uint16_t>(bytes[2 * word] << 8 | bytes[2 * word + 1]);
  }
  return words;
}

std::optional<HighScoreTable> unpackHighScores(const StorageWords &words) {
  std::array<uint8_t, STORAGE_BYTES> bytes{};
  for (std::size_t word = 0; word < STORAGE_WORDS; ++word) {
    bytes[2 * word] = static_cast<uint8_t>(words[word] >> 8);
    bytes[2 * word + 1] = static_cast<uint8_t>(words[word]);
  }
  if (bytes[0] != MAGIC) {
    return std::nullopt;
  }
  BitReader reader(bytes);
  HighScoreTable::Bytes table{};
  for (int row = 0; row < HighScoreTable::ROWS; ++row) {
    for (int column = 0; column < HighScoreTable::NAME_LENGTH; ++column) {
      table[static_cast<std::size_t>(row * HighScoreTable::RECORD_SIZE +
                                     column)] =
          letterValue(static_cast<uint8_t>(reader.get(CODE_BITS)));
    }
    table[static_cast<std::size_t>(row * HighScoreTable::RECORD_SIZE +
                                   HighScoreTable::NAME_LENGTH)] =
        static_cast<uint8_t>(reader.get(SCORE_BITS));
  }
  return HighScoreTable::fromBytes(table);
}

} // namespace openfranko::src::engine::street::core
