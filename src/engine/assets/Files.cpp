#include "Files.h"

#include <utility>

namespace openfranko::src::engine::assets {
namespace {

class PathListing : public Files::Listing {
public:
  explicit PathListing(std::vector<std::string> paths)
      : m_paths(std::move(paths)) {}

  bool next(std::string_view &name) override {
    if (m_next >= m_paths.size()) {
      return false;
    }
    name = m_paths[m_next++];
    const std::size_t slash = name.find_last_of("/\\");
    if (slash != std::string_view::npos) {
      name.remove_prefix(slash + 1);
    }
    return true;
  }

private:
  std::vector<std::string> m_paths;
  std::size_t m_next = 0;
};

class WholeBitmap : public Files::BitmapLoad {
public:
  WholeBitmap(Files &files, std::string path)
      : m_files(files), m_path(std::move(path)) {}

  bool step(systems::graphics::IndexedBitmap &bitmap) override {
    bitmap = m_files.loadBitmap(m_path);
    return true;
  }

private:
  Files &m_files;
  std::string m_path;
};

class WholeFile : public Files::FileLoad {
public:
  WholeFile(Files &files, std::string path)
      : m_files(files), m_path(std::move(path)) {}

  bool step(std::vector<uint8_t> &data) override {
    data = m_files.read(m_path);
    return true;
  }

private:
  Files &m_files;
  std::string m_path;
};

} // namespace

std::unique_ptr<Files::FileLoad> Files::beginRead(const std::string &path) {
  return std::make_unique<WholeFile>(*this, path);
}

std::unique_ptr<Files::BitmapLoad> Files::beginBitmap(const std::string &path) {
  return std::make_unique<WholeBitmap>(*this, path);
}

std::unique_ptr<Files::Listing>
Files::walk(const std::string &directory) const {
  return std::make_unique<PathListing>(list(directory));
}

} // namespace openfranko::src::engine::assets
