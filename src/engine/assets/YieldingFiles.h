#ifndef ENGINE_ASSETS_YIELDINGFILES_H_
#define ENGINE_ASSETS_YIELDINGFILES_H_

#include "Files.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

class YieldingFiles : public Files {
public:
  YieldingFiles(std::unique_ptr<Files> files, std::function<void()> yield);

  bool exists(const std::string &path) const override;
  std::vector<std::string> list(const std::string &directory) const override;
  std::unique_ptr<Listing> walk(const std::string &directory) const override;
  systems::graphics::IndexedBitmap loadBitmap(const std::string &path) override;
  std::unique_ptr<BitmapLoad> beginBitmap(const std::string &path) override;
  std::vector<uint8_t> read(const std::string &path) override;
  std::unique_ptr<FileLoad> beginRead(const std::string &path) override;

private:
  std::unique_ptr<Files> m_files;
  std::function<void()> m_yield;
};

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_YIELDINGFILES_H_
