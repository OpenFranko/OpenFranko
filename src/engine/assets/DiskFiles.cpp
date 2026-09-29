#include "DiskFiles.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace openfranko::src::engine::assets {

bool DiskFiles::exists(const std::string &path) const {
  return std::filesystem::exists(path);
}

std::vector<std::string> DiskFiles::list(const std::string &directory) const {
  std::vector<std::string> paths;
  std::error_code error;
  for (const auto &entry :
       std::filesystem::directory_iterator(directory, error)) {
    paths.push_back(entry.path().string());
  }
  return paths;
}

systems::graphics::IndexedBitmap
DiskFiles::loadBitmap(const std::string &path) {
  return systems::graphics::loadIndexedBitmap(path);
}

std::vector<uint8_t> DiskFiles::read(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open " + path);
  }
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

} // namespace openfranko::src::engine::assets
