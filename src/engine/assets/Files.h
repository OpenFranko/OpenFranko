#ifndef ENGINE_ASSETS_FILES_H_
#define ENGINE_ASSETS_FILES_H_

#include "../../systems/graphics/Bitmap.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

class Files {
public:
  class Listing {
  public:
    virtual ~Listing() = default;
    virtual bool next(std::string_view &name) = 0;
  };

  class BitmapLoad {
  public:
    virtual ~BitmapLoad() = default;
    virtual bool step(systems::graphics::IndexedBitmap &bitmap) = 0;
  };

  class FileLoad {
  public:
    virtual ~FileLoad() = default;
    virtual bool step(std::vector<uint8_t> &data) = 0;
  };

  virtual ~Files() = default;

  virtual bool exists(const std::string &path) const = 0;
  virtual std::vector<std::string> list(const std::string &directory) const = 0;
  virtual std::unique_ptr<Listing> walk(const std::string &directory) const;
  virtual systems::graphics::IndexedBitmap
  loadBitmap(const std::string &path) = 0;
  virtual std::unique_ptr<BitmapLoad> beginBitmap(const std::string &path);
  virtual std::vector<uint8_t> read(const std::string &path) = 0;
  virtual std::unique_ptr<FileLoad> beginRead(const std::string &path);
};

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_FILES_H_
