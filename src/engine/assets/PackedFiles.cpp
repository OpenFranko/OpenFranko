#include "PackedFiles.h"

#include "Lz4.h"
#include "PackedArchive.h"

#include <cstring>
#include <stdexcept>

namespace openfranko::src::engine::assets {
namespace {

std::string normalized(std::string path) {
  while (path.compare(0, 2, "./") == 0) {
    path.erase(0, 2);
  }
  while (!path.empty() && path.back() == '/') {
    path.pop_back();
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
  const std::string name = normalized(path);
  const std::size_t found = lowerBound(name);
  if (found < m_count && name == nameAt(found)) {
    return true;
  }
  const std::string directory = name + "/";
  const std::size_t child = lowerBound(directory);
  return child < m_count && startsWith(nameAt(child), directory);
}

std::vector<std::string> PackedFiles::list(const std::string &directory) const {
  const std::string prefix = normalized(directory) + "/";
  std::vector<std::string> paths;
  for (std::size_t index = lowerBound(prefix);
       index < m_count && startsWith(nameAt(index), prefix); ++index) {
    const std::string path = nameAt(index);
    const std::size_t slash = path.find('/', prefix.size());
    if (slash == std::string::npos) {
      paths.push_back(path);
    } else if (paths.empty() || paths.back().size() != slash ||
               path.compare(0, slash, paths.back()) != 0) {
      paths.push_back(path.substr(0, slash));
    }
  }
  return paths;
}

systems::graphics::IndexedBitmap
PackedFiles::loadBitmap(const std::string &path) {
  if (!exists(path)) {
    throw std::runtime_error("Failed to load bitmap: " + path);
  }
  const Entry found = require(path);
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
    decompressLz4(stored, storedPixels, bitmap.pixels.data(), pixels);
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
    decompressLz4(found.data, found.storedSize, data.data(), data.size());
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

std::size_t PackedFiles::lowerBound(const std::string &name) const {
  std::size_t low = 0;
  std::size_t high = m_count;
  while (low < high) {
    const std::size_t middle = low + (high - low) / 2;
    if (std::strcmp(nameAt(middle), name.c_str()) < 0) {
      low = middle + 1;
    } else {
      high = middle;
    }
  }
  return low;
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
  const std::string name = normalized(path);
  const std::size_t found = lowerBound(name);
  if (found >= m_count || name != nameAt(found)) {
    throw std::runtime_error("Failed to open " + path);
  }
  return entry(found);
}

} // namespace openfranko::src::engine::assets
