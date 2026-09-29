#ifndef ENGINE_ASSETS_FILES_H_
#define ENGINE_ASSETS_FILES_H_

#include "../../systems/graphics/Bitmap.h"

#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

class Files {
public:
  virtual ~Files() = default;

  virtual bool exists(const std::string &path) const = 0;
  virtual std::vector<std::string> list(const std::string &directory) const = 0;
  virtual systems::graphics::IndexedBitmap
  loadBitmap(const std::string &path) = 0;
  virtual std::vector<uint8_t> read(const std::string &path) = 0;
};

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_FILES_H_
