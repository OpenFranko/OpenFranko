#ifndef TEST_SRC_ENGINE_ASSETS_FAKEFILES_H_
#define TEST_SRC_ENGINE_ASSETS_FAKEFILES_H_

#include "../../../../src/engine/assets/Files.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace openfranko {
namespace test {
namespace src {
namespace engine {
namespace assets {

class FakeFiles : public openfranko::src::engine::assets::Files {
public:
  std::map<std::string, openfranko::src::systems::graphics::IndexedBitmap>
      bitmaps;
  std::map<std::string, std::vector<uint8_t>> contents;
  std::vector<std::string> loaded;

  bool exists(const std::string &path) const override {
    return bitmaps.count(path) != 0 || contents.count(path) != 0;
  }

  std::vector<std::string> list(const std::string &directory) const override {
    std::vector<std::string> paths;
    for (const auto &bitmap : bitmaps) {
      addIfIn(directory, bitmap.first, paths);
    }
    for (const auto &content : contents) {
      addIfIn(directory, content.first, paths);
    }
    return paths;
  }

  openfranko::src::systems::graphics::IndexedBitmap
  loadBitmap(const std::string &path) override {
    loaded.push_back(path);
    const auto found = bitmaps.find(path);
    return found != bitmaps.end()
               ? found->second
               : openfranko::src::systems::graphics::IndexedBitmap{};
  }

  std::vector<uint8_t> read(const std::string &path) override {
    loaded.push_back(path);
    return contents.at(path);
  }

  bool wasLoaded(const std::string &path) const {
    return std::find(loaded.begin(), loaded.end(), path) != loaded.end();
  }

private:
  static void addIfIn(const std::string &directory, const std::string &path,
                      std::vector<std::string> &paths) {
    if (std::filesystem::path(path).parent_path() ==
        std::filesystem::path(directory)) {
      paths.push_back(path);
    }
  }
};

} // namespace assets
} // namespace engine
} // namespace src
} // namespace test
} // namespace openfranko

#endif // TEST_SRC_ENGINE_ASSETS_FAKEFILES_H_
