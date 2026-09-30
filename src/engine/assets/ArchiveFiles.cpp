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
  std::vector<std::string> paths;
  for (auto entry = m_entries.lower_bound(prefix);
       entry != m_entries.end() &&
       entry->first.compare(0, prefix.size(), prefix) == 0;
       ++entry) {
    const std::string child =
        entry->first.substr(0, entry->first.find('/', prefix.size()));
    if (paths.empty() || paths.back() != child) {
      paths.push_back(child);
    }
  }
  return paths;
}

systems::graphics::IndexedBitmap
ArchiveFiles::loadBitmap(const std::string &path) {
  if (m_entries.count(normalized(path)) == 0) {
    throw std::runtime_error("Failed to load bitmap: " + path);
  }
  try {
    return systems::graphics::readIndexedBitmap(read(path));
  } catch (const std::runtime_error &error) {
    throw std::runtime_error(std::string(error.what()) + ": " + path);
  }
}

std::vector<uint8_t> ArchiveFiles::read(const std::string &path) {
  const auto entry = m_entries.find(normalized(path));
  if (entry == m_entries.end()) {
    throw std::runtime_error("Failed to open " + path);
  }
  std::vector<uint8_t> data(entry->second.size);
  m_archive.seekg(static_cast<std::streamoff>(entry->second.offset));
  if (!m_archive.read(reinterpret_cast<char *>(data.data()),
                      static_cast<std::streamsize>(data.size()))) {
    m_archive.clear();
    throw std::runtime_error("Truncated asset archive entry: " + path);
  }
  return data;
}

} // namespace openfranko::src::engine::assets
