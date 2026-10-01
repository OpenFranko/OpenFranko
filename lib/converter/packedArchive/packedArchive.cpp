#include "packedArchive.h"

#include "../../../src/engine/assets/PackedArchive.h"
#include "lz4Compressor.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace openfranko::lib::converter::packedArchive {
namespace {

namespace packed = src::engine::assets::packed;

constexpr auto BITMAP_EXTENSION = ".bmp";

void putLong(std::vector<uint8_t> &out, std::size_t at, uint32_t value) {
  out[at] = static_cast<uint8_t>(value >> 24);
  out[at + 1] = static_cast<uint8_t>(value >> 16);
  out[at + 2] = static_cast<uint8_t>(value >> 8);
  out[at + 3] = static_cast<uint8_t>(value);
}

void pushWord(std::vector<uint8_t> &out, uint16_t value) {
  out.push_back(static_cast<uint8_t>(value >> 8));
  out.push_back(static_cast<uint8_t>(value));
}

void align(std::vector<uint8_t> &out) {
  while (out.size() % packed::DATA_ALIGNMENT != 0) {
    out.push_back(0);
  }
}

std::vector<uint8_t> readAll(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open " + path.string());
  }
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

} // namespace

void ArchiveWriter::addFile(const std::string &name,
                            const std::vector<uint8_t> &data) {
  Entry entry;
  entry.name = name;
  entry.size = static_cast<uint32_t>(data.size());
  std::vector<uint8_t> compressed = compressLz4(data.data(), data.size());
  if (compressed.size() < data.size()) {
    entry.stored = std::move(compressed);
    entry.flags = packed::COMPRESSED;
  } else {
    entry.stored = data;
  }
  m_entries.push_back(std::move(entry));
}

void ArchiveWriter::addBitmap(
    const std::string &name,
    const src::systems::graphics::IndexedBitmap &bitmap) {
  Entry entry;
  entry.name = name;
  entry.flags = packed::BITMAP;
  pushWord(entry.stored, static_cast<uint16_t>(bitmap.width));
  pushWord(entry.stored, static_cast<uint16_t>(bitmap.height));
  pushWord(entry.stored, static_cast<uint16_t>(bitmap.hotspotX));
  pushWord(entry.stored, static_cast<uint16_t>(bitmap.hotspotY));
  pushWord(entry.stored, static_cast<uint16_t>(bitmap.palette.size()));
  pushWord(entry.stored, 0);
  for (const uint16_t color : bitmap.palette) {
    pushWord(entry.stored, color);
  }
  entry.size =
      static_cast<uint32_t>(entry.stored.size() + bitmap.pixels.size());
  std::vector<uint8_t> compressed =
      compressLz4(bitmap.pixels.data(), bitmap.pixels.size());
  if (compressed.size() < bitmap.pixels.size()) {
    entry.stored.insert(entry.stored.end(), compressed.begin(),
                        compressed.end());
    entry.flags |= packed::COMPRESSED;
  } else {
    entry.stored.insert(entry.stored.end(), bitmap.pixels.begin(),
                        bitmap.pixels.end());
  }
  m_entries.push_back(std::move(entry));
}

std::vector<uint8_t> ArchiveWriter::finish() const {
  std::vector<const Entry *> sorted;
  for (const Entry &entry : m_entries) {
    sorted.push_back(&entry);
  }
  std::sort(sorted.begin(), sorted.end(),
            [](const Entry *left, const Entry *right) {
              return left->name < right->name;
            });
  for (std::size_t i = 1; i < sorted.size(); ++i) {
    if (sorted[i]->name == sorted[i - 1]->name) {
      throw std::runtime_error("Duplicate archive entry: " + sorted[i]->name);
    }
  }
  std::vector<uint8_t> out(packed::HEADER_SIZE +
                           sorted.size() * packed::ENTRY_SIZE);
  putLong(out, packed::MAGIC_OFFSET, packed::MAGIC);
  putLong(out, packed::VERSION_OFFSET, packed::VERSION);
  putLong(out, packed::COUNT_OFFSET, static_cast<uint32_t>(sorted.size()));
  for (std::size_t i = 0; i < sorted.size(); ++i) {
    const std::size_t at = packed::HEADER_SIZE + i * packed::ENTRY_SIZE;
    putLong(out, at + packed::NAME_OFFSET, static_cast<uint32_t>(out.size()));
    out.insert(out.end(), sorted[i]->name.begin(), sorted[i]->name.end());
    out.push_back(0);
  }
  for (std::size_t i = 0; i < sorted.size(); ++i) {
    align(out);
    const std::size_t at = packed::HEADER_SIZE + i * packed::ENTRY_SIZE;
    const Entry &entry = *sorted[i];
    putLong(out, at + packed::DATA_OFFSET, static_cast<uint32_t>(out.size()));
    putLong(out, at + packed::STORED_SIZE_OFFSET,
            static_cast<uint32_t>(entry.stored.size()));
    putLong(out, at + packed::UNPACKED_SIZE_OFFSET, entry.size);
    putLong(out, at + packed::FLAGS_OFFSET, entry.flags);
    out.insert(out.end(), entry.stored.begin(), entry.stored.end());
  }
  align(out);
  putLong(out, packed::SIZE_OFFSET, static_cast<uint32_t>(out.size()));
  return out;
}

std::vector<uint8_t> packDirectory(const std::string &directory,
                                   const std::string &prefix) {
  ArchiveWriter writer;
  const std::filesystem::path root(directory);
  if (!std::filesystem::is_directory(root)) {
    throw std::runtime_error("Not a directory: " + directory);
  }
  for (const auto &item : std::filesystem::recursive_directory_iterator(root)) {
    if (!item.is_regular_file()) {
      continue;
    }
    const std::string relative =
        std::filesystem::relative(item.path(), root).generic_string();
    const std::string name =
        prefix.empty() ? relative : prefix + "/" + relative;
    const std::vector<uint8_t> data = readAll(item.path());
    if (item.path().extension() == BITMAP_EXTENSION) {
      writer.addBitmap(name, src::systems::graphics::readIndexedBitmap(data));
    } else {
      writer.addFile(name, data);
    }
  }
  return writer.finish();
}

} // namespace openfranko::lib::converter::packedArchive
