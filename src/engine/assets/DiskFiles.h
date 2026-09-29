#ifndef ENGINE_ASSETS_DISKFILES_H_
#define ENGINE_ASSETS_DISKFILES_H_

#include "Files.h"

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

class DiskFiles : public Files {
public:
  bool exists(const std::string &path) const override;
  std::vector<std::string> list(const std::string &directory) const override;
  systems::graphics::IndexedBitmap loadBitmap(const std::string &path) override;
  std::vector<uint8_t> read(const std::string &path) override;
};

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_DISKFILES_H_
