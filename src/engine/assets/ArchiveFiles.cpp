#include "ArchiveFiles.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>

namespace openfranko::src::engine::assets {
namespace {

constexpr std::size_t BLOCK_SIZE = 512;
constexpr std::size_t NAME_OFFSET = 0;
constexpr std::size_t NAME_SIZE = 100;
constexpr std::size_t LENGTH_OFFSET = 124;
constexpr std::size_t LENGTH_SIZE = 12;
constexpr std::size_t TYPE_OFFSET = 156;
constexpr std::size_t MAGIC_OFFSET = 257;
constexpr std::size_t MAGIC_SIZE = 5;
constexpr std::size_t PREFIX_OFFSET = 345;
constexpr std::size_t PREFIX_SIZE = 155;
constexpr auto MAGIC = "ustar";
constexpr char REGULAR_FILE = '0';
constexpr char OLD_REGULAR_FILE = '\0';
constexpr char PAST_SEPARATOR = '/' + 1;

using Block = std::array<char, BLOCK_SIZE>;

std::string_view field(const Block &block, std::size_t offset,
                       std::size_t size) {
  const char *start = block.data() + offset;
  const char *end = std::find(start, start + size, '\0');
  return std::string_view(start, static_cast<std::size_t>(end - start));
}

std::size_t octal(std::string_view digits) {
  std::size_t value = 0;
  for (const char digit : digits) {
    if (digit >= '0' && digit <= '7') {
      value = value * 8 + static_cast<std::size_t>(digit - '0');
    }
  }
  return value;
}

std::string entryPath(const Block &block) {
  std::string path(field(block, PREFIX_OFFSET, PREFIX_SIZE));
  if (!path.empty()) {
    path += '/';
  }
  path += field(block, NAME_OFFSET, NAME_SIZE);
  return path;
}

std::string normalized(std::string path) {
  while (path.compare(0, 2, "./") == 0) {
    path.erase(0, 2);
  }
  while (!path.empty() && path.back() == '/') {
    path.pop_back();
  }
  return path;
}

} // namespace

ArchiveFiles::ArchiveFiles(const std::string &path) {
  m_archive.rdbuf()->pubsetbuf(nullptr, 0);
  m_archive.open(path, std::ios::binary);
  if (!m_archive) {
    throw std::runtime_error("Failed to open asset archive: " + path);
  }
  Block block{};
  std::size_t offset = 0;
  while (m_archive.read(block.data(), BLOCK_SIZE)) {
    offset += BLOCK_SIZE;
    if (field(block, NAME_OFFSET, NAME_SIZE).empty()) {
      break;
    }
    if (field(block, MAGIC_OFFSET, MAGIC_SIZE) != MAGIC) {
      throw std::runtime_error("Not a tar archive: " + path);
    }
    const std::size_t length = octal(field(block, LENGTH_OFFSET, LENGTH_SIZE));
    const char type = block[TYPE_OFFSET];
    if (type == REGULAR_FILE || type == OLD_REGULAR_FILE) {
      m_entries.insert_or_assign(m_entries.end(), normalized(entryPath(block)),
                                 Entry{offset, length});
    }
    offset += (length + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE;
    m_archive.seekg(static_cast<std::streamoff>(offset));
  }
  m_archive.clear();
}

bool ArchiveFiles::exists(const std::string &path) const {
  const std::string name = normalized(path);
  if (m_entries.count(name) != 0) {
    return true;
  }
  const std::string directory = name + "/";
  const auto entry = m_entries.lower_bound(directory);
  return entry != m_entries.end() &&
         entry->first.compare(0, directory.size(), directory) == 0;
}

std::vector<std::string>
ArchiveFiles::list(const std::string &directory) const {
  const std::string prefix = normalized(directory) + "/";
  std::string afterPrefix = prefix;
  afterPrefix.back() = PAST_SEPARATOR;
  const auto last = m_entries.lower_bound(afterPrefix);
  std::vector<std::string> paths;
  for (auto entry = m_entries.lower_bound(prefix); entry != last; ++entry) {
    const std::string &path = entry->first;
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
ArchiveFiles::loadBitmap(const std::string &path) {
  const Entry *entry = find(path);
  if (!entry) {
    throw std::runtime_error("Failed to load bitmap: " + path);
  }
  if (m_bitmapFile.size() < entry->size) {
    m_bitmapFile.resize(entry->size);
  }
  readInto(*entry, m_bitmapFile.data(), path);
  try {
    return systems::graphics::readIndexedBitmap(m_bitmapFile.data(),
                                                entry->size);
  } catch (const std::runtime_error &error) {
    throw std::runtime_error(std::string(error.what()) + ": " + path);
  }
}

std::vector<uint8_t> ArchiveFiles::read(const std::string &path) {
  const Entry *entry = find(path);
  if (!entry) {
    throw std::runtime_error("Failed to open " + path);
  }
  return contents(*entry, path);
}

const ArchiveFiles::Entry *ArchiveFiles::find(const std::string &path) const {
  const auto found = m_entries.find(normalized(path));
  return found != m_entries.end() ? &found->second : nullptr;
}

std::vector<uint8_t> ArchiveFiles::contents(const Entry &entry,
                                            const std::string &path) {
  std::vector<uint8_t> data(entry.size);
  readInto(entry, data.data(), path);
  return data;
}

void ArchiveFiles::readInto(const Entry &entry, uint8_t *target,
                            const std::string &path) {
  m_archive.seekg(static_cast<std::streamoff>(entry.offset));
  if (!m_archive.read(reinterpret_cast<char *>(target),
                      static_cast<std::streamsize>(entry.size))) {
    m_archive.clear();
    throw std::runtime_error("Truncated asset archive entry: " + path);
  }
}

} // namespace openfranko::src::engine::assets
