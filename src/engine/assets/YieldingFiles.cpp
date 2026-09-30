#include "YieldingFiles.h"

#include <utility>

namespace openfranko::src::engine::assets {

YieldingFiles::YieldingFiles(std::unique_ptr<Files> files,
                             std::function<void()> yield)
    : m_files(std::move(files)), m_yield(std::move(yield)) {}

bool YieldingFiles::exists(const std::string &path) const {
  return m_files->exists(path);
}

std::vector<std::string>
YieldingFiles::list(const std::string &directory) const {
  return m_files->list(directory);
}

systems::graphics::IndexedBitmap
YieldingFiles::loadBitmap(const std::string &path) {
  systems::graphics::IndexedBitmap bitmap = m_files->loadBitmap(path);
  m_yield();
  return bitmap;
}

std::vector<uint8_t> YieldingFiles::read(const std::string &path) {
  std::vector<uint8_t> data = m_files->read(path);
  m_yield();
  return data;
}

} // namespace openfranko::src::engine::assets
