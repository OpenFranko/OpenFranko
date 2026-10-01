#include "lz4Compressor.h"

#include <algorithm>

namespace openfranko::lib::converter::packedArchive {
namespace {

constexpr int HASH_BITS = 16;
constexpr std::size_t MIN_MATCH = 4;
constexpr std::size_t MATCH_FIND_LIMIT = 12;
constexpr std::size_t LAST_LITERALS = 5;
constexpr std::size_t MAX_OFFSET = 65535;
constexpr int MAX_ATTEMPTS = 1024;
constexpr std::size_t RUN_MASK = 15;
constexpr uint8_t EXTENSION_LIMIT = 255;
constexpr uint32_t HASH_MULTIPLIER = 2654435761u;
constexpr long NO_POSITION = -1;

class Matcher {
public:
  Matcher(const uint8_t *data, std::size_t size)
      : m_data(data), m_size(size),
        m_head(std::size_t(1) << HASH_BITS, NO_POSITION),
        m_chain(size, NO_POSITION) {}

  void insert(std::size_t position) {
    if (position + MIN_MATCH > m_size || position < m_inserted) {
      return;
    }
    while (m_inserted <= position) {
      if (m_inserted + MIN_MATCH <= m_size) {
        const uint32_t key = hash(m_inserted);
        m_chain[m_inserted] = m_head[key];
        m_head[key] = static_cast<long>(m_inserted);
      }
      ++m_inserted;
    }
  }

  std::size_t find(std::size_t position, std::size_t &offset) const {
    if (position + MATCH_FIND_LIMIT > m_size) {
      return 0;
    }
    const std::size_t limit = m_size - LAST_LITERALS - position;
    std::size_t best = 0;
    long candidate = m_head[hash(position)];
    for (int attempt = 0; attempt < MAX_ATTEMPTS && candidate != NO_POSITION;
         ++attempt) {
      const std::size_t from = static_cast<std::size_t>(candidate);
      if (from >= position) {
        candidate = m_chain[from];
        continue;
      }
      if (position - from > MAX_OFFSET) {
        break;
      }
      std::size_t length = 0;
      while (length < limit &&
             m_data[from + length] == m_data[position + length]) {
        ++length;
      }
      if (length > best) {
        best = length;
        offset = position - from;
        if (length == limit) {
          break;
        }
      }
      candidate = m_chain[from];
    }
    return best >= MIN_MATCH ? best : 0;
  }

private:
  uint32_t hash(std::size_t position) const {
    const uint32_t value = static_cast<uint32_t>(m_data[position]) |
                           static_cast<uint32_t>(m_data[position + 1]) << 8 |
                           static_cast<uint32_t>(m_data[position + 2]) << 16 |
                           static_cast<uint32_t>(m_data[position + 3]) << 24;
    return (value * HASH_MULTIPLIER) >> (32 - HASH_BITS);
  }

  const uint8_t *m_data;
  std::size_t m_size;
  std::vector<long> m_head;
  std::vector<long> m_chain;
  std::size_t m_inserted = 0;
};

void writeLength(std::vector<uint8_t> &out, std::size_t length) {
  while (length >= EXTENSION_LIMIT) {
    out.push_back(EXTENSION_LIMIT);
    length -= EXTENSION_LIMIT;
  }
  out.push_back(static_cast<uint8_t>(length));
}

void writeSequence(std::vector<uint8_t> &out, const uint8_t *literals,
                   std::size_t literalCount, std::size_t offset,
                   std::size_t matchLength) {
  const std::size_t literalNibble = std::min(literalCount, RUN_MASK);
  const std::size_t matchCode = matchLength == 0 ? 0 : matchLength - MIN_MATCH;
  const std::size_t matchNibble = std::min(matchCode, RUN_MASK);
  out.push_back(static_cast<uint8_t>(literalNibble << 4 | matchNibble));
  if (literalCount >= RUN_MASK) {
    writeLength(out, literalCount - RUN_MASK);
  }
  out.insert(out.end(), literals, literals + literalCount);
  if (matchLength == 0) {
    return;
  }
  out.push_back(static_cast<uint8_t>(offset & 0xFF));
  out.push_back(static_cast<uint8_t>(offset >> 8));
  if (matchCode >= RUN_MASK) {
    writeLength(out, matchCode - RUN_MASK);
  }
}

} // namespace

std::vector<uint8_t> compressLz4(const uint8_t *data, std::size_t size) {
  std::vector<uint8_t> out;
  Matcher matcher(data, size);
  std::size_t literalStart = 0;
  std::size_t position = 0;
  while (position + MATCH_FIND_LIMIT <= size) {
    std::size_t offset = 0;
    matcher.insert(position);
    const std::size_t length = matcher.find(position, offset);
    if (length == 0) {
      ++position;
      continue;
    }
    matcher.insert(position + 1);
    std::size_t nextOffset = 0;
    const std::size_t next = matcher.find(position + 1, nextOffset);
    if (next > length + 1) {
      ++position;
      continue;
    }
    writeSequence(out, data + literalStart, position - literalStart, offset,
                  length);
    position += length;
    matcher.insert(position - 1);
    literalStart = position;
  }
  writeSequence(out, data + literalStart, size - literalStart, 0, 0);
  return out;
}

} // namespace openfranko::lib::converter::packedArchive
