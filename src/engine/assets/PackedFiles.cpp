#include "PackedFiles.h"

#include "Lz4.h"
#include "PackedArchive.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace openfranko::src::engine::assets {
namespace {

std::string_view normalized(std::string_view path) {
  while (path.compare(0, 2, "./") == 0) {
    path.remove_prefix(2);
  }
  while (!path.empty() && path.back() == '/') {
    path.remove_suffix(1);
  }
  return path;
}

bool startsWith(const char *text, const std::string &prefix) {
  return std::strncmp(text, prefix.c_str(), prefix.size()) == 0;
}

[[noreturn]] void corrupt() {
  throw std::runtime_error("Corrupt asset archive");
}

} // namespace

PackedFiles::PackedFiles(const uint8_t *data, std::size_t size)
    : m_data(data), m_size(size), m_count(0) {
  if (size < packed::HEADER_SIZE ||
      packed::readLong(data + packed::MAGIC_OFFSET) != packed::MAGIC ||
      packed::readLong(data + packed::VERSION_OFFSET) != packed::VERSION) {
    throw std::runtime_error("Not an OpenFranko asset archive");
  }
  m_count = packed::readLong(data + packed::COUNT_OFFSET);
  const std::size_t stated = packed::readLong(data + packed::SIZE_OFFSET);
  if (stated > size ||
      m_count > (stated - packed::HEADER_SIZE) / packed::ENTRY_SIZE) {
    corrupt();
  }
  m_size = stated;
}

std::size_t PackedFiles::entries() const { return m_count; }

bool PackedFiles::exists(const std::string &path) const {
  const std::string_view name = normalized(path);
  if (indexOf(name) < m_count) {
    return true;
  }
  const std::string directory = std::string(name) + "/";
  const std::size_t child = lowerBound(directory);
  return child < m_count && startsWith(nameAt(child), directory);
}

class PackedFiles::Walk : public Files::Listing {
public:
  Walk(const PackedFiles &files, const std::string &directory)
      : m_files(files), m_prefix(std::string(normalized(directory)) + "/"),
        m_index(files.lowerBound(m_prefix)), m_end(files.lowerBound(after())) {}

  bool next(std::string_view &name) override {
    while (m_index < m_end) {
      const char *entry = m_files.nameAt(m_index++) + m_prefix.size();
      const char *end = entry;
      while (*end != '\0' && *end != '/') {
        ++end;
      }
      name = std::string_view(entry, static_cast<std::size_t>(end - entry));
      if (*end == '\0') {
        return true;
      }
      if (name == m_folder) {
        continue;
      }
      m_folder = name;
      return true;
    }
    return false;
  }

  const std::string &prefix() const { return m_prefix; }

private:
  std::string after() const {
    std::string bound = m_prefix;
    ++bound.back();
    return bound;
  }

  const PackedFiles &m_files;
  std::string m_prefix;
  std::size_t m_index;
  std::size_t m_end;
  std::string_view m_folder;
};

std::vector<std::string> PackedFiles::list(const std::string &directory) const {
  Walk walk(*this, directory);
  std::vector<std::string> paths;
  std::string_view name;
  while (walk.next(name)) {
    paths.push_back(walk.prefix() + std::string(name));
  }
  return paths;
}

std::unique_ptr<Files::Listing>
PackedFiles::walk(const std::string &directory) const {
  return std::make_unique<Walk>(*this, directory);
}

systems::graphics::IndexedBitmap
PackedFiles::loadBitmap(const std::string &path) {
  const std::size_t index = indexOf(normalized(path));
  if (index >= m_count) {
    if (exists(path)) {
      throw std::runtime_error("Failed to open " + path);
    }
    throw std::runtime_error("Failed to load bitmap: " + path);
  }
  const Entry found = entry(index);
  if ((found.flags & packed::BITMAP) == 0 ||
      found.storedSize < packed::BITMAP_HEADER_SIZE) {
    throw std::runtime_error("Failed to load bitmap: " + path);
  }
  const uint8_t *header = found.data;
  systems::graphics::IndexedBitmap bitmap;
  bitmap.width = packed::readWord(header + packed::BITMAP_WIDTH_OFFSET);
  bitmap.height = packed::readWord(header + packed::BITMAP_HEIGHT_OFFSET);
  bitmap.hotspotX = static_cast<int16_t>(
      packed::readWord(header + packed::BITMAP_HOTSPOT_X_OFFSET));
  bitmap.hotspotY = static_cast<int16_t>(
      packed::readWord(header + packed::BITMAP_HOTSPOT_Y_OFFSET));
  const std::size_t colors =
      packed::readWord(header + packed::BITMAP_COLORS_OFFSET);
  const std::size_t paletteEnd = packed::BITMAP_HEADER_SIZE + 2 * colors;
  if (paletteEnd > found.storedSize) {
    corrupt();
  }
  bitmap.palette.resize(colors);
  for (std::size_t color = 0; color < colors; ++color) {
    bitmap.palette[color] =
        packed::readWord(header + packed::BITMAP_HEADER_SIZE + 2 * color);
  }
  const std::size_t pixels = static_cast<std::size_t>(bitmap.width) *
                             static_cast<std::size_t>(bitmap.height);
  bitmap.pixels.resize(pixels);
  const uint8_t *stored = header + paletteEnd;
  const std::size_t storedPixels = found.storedSize - paletteEnd;
  if (found.flags & packed::COMPRESSED) {
    unpackLz4(stored, storedPixels, bitmap.pixels.data(), pixels);
  } else if (storedPixels == pixels) {
    std::memcpy(bitmap.pixels.data(), stored, pixels);
  } else {
    corrupt();
  }
  return bitmap;
}

std::vector<uint8_t> PackedFiles::read(const std::string &path) {
  const Entry found = require(path);
  if (found.flags & packed::BITMAP) {
    throw std::runtime_error("Failed to open " + path);
  }
  std::vector<uint8_t> data(found.size);
  if (found.flags & packed::COMPRESSED) {
    unpackLz4(found.data, found.storedSize, data.data(), data.size());
  } else if (found.storedSize == found.size) {
    std::memcpy(data.data(), found.data, found.size);
  } else {
    corrupt();
  }
  return data;
}

PackedFiles::Entry PackedFiles::entry(std::size_t index) const {
  const uint8_t *at = m_data + packed::HEADER_SIZE + index * packed::ENTRY_SIZE;
  Entry found;
  found.name = nameAt(index);
  const std::size_t offset = packed::readLong(at + packed::DATA_OFFSET);
  found.storedSize = packed::readLong(at + packed::STORED_SIZE_OFFSET);
  found.size = packed::readLong(at + packed::UNPACKED_SIZE_OFFSET);
  found.flags = packed::readLong(at + packed::FLAGS_OFFSET);
  if (offset > m_size || found.storedSize > m_size - offset) {
    corrupt();
  }
  found.data = m_data + offset;
  return found;
}

std::size_t PackedFiles::lowerBound(std::string_view name) const {
  std::size_t low = 0;
  std::size_t high = m_count;
  std::size_t lowShared = 0;
  std::size_t highShared = 0;
  while (low < high) {
    const std::size_t middle = low + (high - low) / 2;
    const char *entry = nameAt(middle);
    std::size_t shared = std::min(lowShared, highShared);
    while (shared < name.size() && entry[shared] != '\0' &&
           entry[shared] == name[shared]) {
      ++shared;
    }
    if (shared < name.size() && static_cast<unsigned char>(entry[shared]) <
                                    static_cast<unsigned char>(name[shared])) {
      low = middle + 1;
      lowShared = shared;
    } else {
      high = middle;
      highShared = shared;
    }
  }
  return low;
}

std::size_t PackedFiles::indexOf(std::string_view name) const {
  std::size_t found = m_next;
  if (found < m_count && isNamed(found, name)) {
    m_next = found + 1;
    return found;
  }
  if (found > 0 && isNamed(found - 1, name)) {
    return found - 1;
  }
  found = lowerBound(name);
  if (found >= m_count || !isNamed(found, name)) {
    return m_count;
  }
  m_next = found + 1;
  return found;
}

bool PackedFiles::isNamed(std::size_t index, std::string_view name) const {
  const char *entry = nameAt(index);
  for (const char letter : name) {
    if (*entry == '\0' || *entry != letter) {
      return false;
    }
    ++entry;
  }
  return *entry == '\0';
}

const char *PackedFiles::nameAt(std::size_t index) const {
  const std::size_t offset =
      packed::readLong(m_data + packed::HEADER_SIZE +
                       index * packed::ENTRY_SIZE + packed::NAME_OFFSET);
  if (offset >= m_size) {
    corrupt();
  }
  return reinterpret_cast<const char *>(m_data + offset);
}

PackedFiles::Entry PackedFiles::require(const std::string &path) const {
  const std::size_t found = indexOf(normalized(path));
  if (found >= m_count) {
    throw std::runtime_error("Failed to open " + path);
  }
  return entry(found);
}

} // namespace openfranko::src::engine::assets
