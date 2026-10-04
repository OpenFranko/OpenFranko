#include "PrefetchingFiles.h"

#include <exception>
#include <utility>

namespace openfranko::src::engine::assets {

PrefetchingFiles::PrefetchingFiles(std::unique_ptr<Files> files)
    : m_files(std::move(files)) {}

void PrefetchingFiles::prefetch(std::vector<std::string> paths) {
  drop();
  m_paths = std::move(paths);
}

void PrefetchingFiles::prefetchMore(const std::vector<std::string> &paths) {
  m_paths.insert(m_paths.end(), paths.begin(), paths.end());
}

bool PrefetchingFiles::step() {
  if (m_next >= m_paths.size()) {
    return false;
  }
  try {
    if (!m_load) {
      m_load = m_files->beginBitmap(m_paths[m_next]);
    }
    if (!m_load->step(m_bitmap)) {
      return true;
    }
    m_ready[m_paths[m_next]] = std::move(m_bitmap);
  } catch (const std::exception &) {
  }
  m_load.reset();
  m_bitmap = systems::graphics::IndexedBitmap{};
  ++m_next;
  return true;
}

void PrefetchingFiles::drop() {
  m_paths.clear();
  m_next = 0;
  m_load.reset();
  m_bitmap = systems::graphics::IndexedBitmap{};
  m_ready.clear();
}

bool PrefetchingFiles::exists(const std::string &path) const {
  return m_files->exists(path);
}

std::vector<std::string>
PrefetchingFiles::list(const std::string &directory) const {
  return m_files->list(directory);
}

std::unique_ptr<Files::Listing>
PrefetchingFiles::walk(const std::string &directory) const {
  return m_files->walk(directory);
}

systems::graphics::IndexedBitmap
PrefetchingFiles::loadBitmap(const std::string &path) {
  const auto found = m_ready.find(path);
  if (found == m_ready.end()) {
    return m_files->loadBitmap(path);
  }
  systems::graphics::IndexedBitmap bitmap = std::move(found->second);
  m_ready.erase(found);
  return bitmap;
}

class PrefetchingFiles::ReadyBitmap : public Files::BitmapLoad {
public:
  explicit ReadyBitmap(systems::graphics::IndexedBitmap bitmap)
      : m_bitmap(std::move(bitmap)) {}

  bool step(systems::graphics::IndexedBitmap &bitmap) override {
    bitmap = std::move(m_bitmap);
    return true;
  }

private:
  systems::graphics::IndexedBitmap m_bitmap;
};

std::unique_ptr<Files::BitmapLoad>
PrefetchingFiles::beginBitmap(const std::string &path) {
  const auto found = m_ready.find(path);
  if (found == m_ready.end()) {
    return m_files->beginBitmap(path);
  }
  auto load = std::make_unique<ReadyBitmap>(std::move(found->second));
  m_ready.erase(found);
  return load;
}

std::vector<uint8_t> PrefetchingFiles::read(const std::string &path) {
  return m_files->read(path);
}

std::unique_ptr<Files::FileLoad>
PrefetchingFiles::beginRead(const std::string &path) {
  return m_files->beginRead(path);
}

} // namespace openfranko::src::engine::assets
