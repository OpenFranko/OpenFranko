#include "graphics/PlanarOverlay.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace openfranko::src::systems::graphics {
namespace {

constexpr uint8_t KEPT = 0xFF;

} // namespace

bool PlanarOverlay::isBuiltFrom(const Overlay &overlay) const {
  return m_built && m_revision == overlay.revision && m_left == overlay.left &&
         m_top == overlay.top && m_width == overlay.width &&
         m_height == overlay.height;
}

void PlanarOverlay::build(const Overlay &overlay) {
  m_built = true;
  m_revision = overlay.revision;
  m_left = overlay.left;
  m_top = overlay.top;
  m_width = overlay.width;
  m_height = overlay.height;
  m_lastWord = 0;
  const auto columnOf = [&](int word, int at, int plane) {
    return (word * WORD_BYTES + at) * PLANES + plane - m_left;
  };
  const auto shows = [&](int row, int column) {
    return column >= 0 && column < m_width &&
           overlay.mask[static_cast<std::size_t>(row * m_width + column)] != 0;
  };
  std::vector<std::size_t> offsets;
  m_words.assign(static_cast<std::size_t>(m_height * PLANES), Words{});
  m_entries.clear();
  const int firstWord = std::max(m_left, 0) / PLANES / WORD_BYTES;
  const int endWord =
      ((m_left + m_width + PLANES - 1) / PLANES + WORD_BYTES - 1) / WORD_BYTES;
  for (int row = 0; row < m_height; ++row) {
    for (int plane = 0; plane < PLANES; ++plane) {
      int first = endWord;
      int last = firstWord;
      for (int word = firstWord; word < endWord; ++word) {
        for (int at = 0; at < WORD_BYTES; ++at) {
          if (shows(row, columnOf(word, at, plane))) {
            first = std::min(first, word);
            last = std::max(last, word + 1);
          }
        }
      }
      Words &words = m_words[static_cast<std::size_t>(row * PLANES + plane)];
      offsets.push_back(m_entries.size());
      if (first >= last) {
        continue;
      }
      words.first = first;
      words.count = last - first;
      m_lastWord = std::max(m_lastWord, last);
      for (int word = first; word < last; ++word) {
        uint8_t entry[ENTRY_BYTES] = {};
        for (int at = 0; at < WORD_BYTES; ++at) {
          const int column = columnOf(word, at, plane);
          entry[WORD_BYTES + at] = KEPT;
          if (shows(row, column)) {
            entry[at] =
                overlay
                    .pixels[static_cast<std::size_t>(row * m_width + column)];
            entry[WORD_BYTES + at] = 0;
          }
        }
        m_entries.insert(m_entries.end(), entry, entry + ENTRY_BYTES);
      }
    }
  }
  for (std::size_t at = 0; at < m_words.size(); ++at) {
    m_words[at].entries = m_entries.data() + offsets[at];
  }
}

void PlanarOverlay::composite(int plane, int row, uint8_t *line) const {
  if (!m_built || row < m_top || row >= m_top + m_height) {
    return;
  }
  const Words &words = *this->words(plane, row);
  const uint8_t *entry = words.entries;
  uint8_t *out = line + words.first * WORD_BYTES;
  for (int word = 0; word < words.count; ++word) {
    uint32_t shown = 0;
    uint32_t value = 0;
    uint32_t keep = 0;
    std::memcpy(&shown, out, sizeof(shown));
    std::memcpy(&value, entry, sizeof(value));
    std::memcpy(&keep, entry + WORD_BYTES, sizeof(keep));
    shown = (shown & keep) | value;
    std::memcpy(out, &shown, sizeof(shown));
    out += WORD_BYTES;
    entry += ENTRY_BYTES;
  }
}

const PlanarOverlay::Words *PlanarOverlay::words(int plane, int row) const {
  return m_words.data() + static_cast<std::ptrdiff_t>(row - m_top) * PLANES +
         plane;
}

int PlanarOverlay::top() const { return m_top; }

int PlanarOverlay::lastWord() const { return m_lastWord; }

} // namespace openfranko::src::systems::graphics
