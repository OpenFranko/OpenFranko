#ifndef ENGINE_ASSETS_PREFETCHINGFILES_H_
#define ENGINE_ASSETS_PREFETCHINGFILES_H_

#include "Files.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

class PrefetchingFiles : public Files {
public:
  explicit PrefetchingFiles(std::unique_ptr<Files> files);

  void prefetch(std::vector<std::string> paths);
  void prefetchMore(const std::vector<std::string> &paths);
  bool step();
  void drop();

  bool exists(const std::string &path) const override;
  std::vector<std::string> list(const std::string &directory) const override;
  std::unique_ptr<Listing> walk(const std::string &directory) const override;
  systems::graphics::IndexedBitmap loadBitmap(const std::string &path) override;
  std::unique_ptr<BitmapLoad> beginBitmap(const std::string &path) override;
  std::vector<uint8_t> read(const std::string &path) override;
  std::unique_ptr<FileLoad> beginRead(const std::string &path) override;

private:
  class ReadyBitmap;

  std::unique_ptr<Files> m_files;
  std::vector<std::string> m_paths;
  std::size_t m_next = 0;
  std::unique_ptr<BitmapLoad> m_load;
  systems::graphics::IndexedBitmap m_bitmap;
  std::map<std::string, systems::graphics::IndexedBitmap> m_ready;
};

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_PREFETCHINGFILES_H_
