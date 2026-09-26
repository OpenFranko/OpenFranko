#ifndef ENGINE_ASSETS_ASSETS_H_
#define ENGINE_ASSETS_ASSETS_H_

#include "../GameVersion.h"

#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

inline constexpr const char *DIRECTORY = "assets";

GameVersion detectVersion(const std::string &directory = DIRECTORY);

std::string resourceName(int resource, GameVersion version);

std::string picturePath(const std::string &name,
                        const std::string &directory = DIRECTORY);

std::string imagePath(const std::string &name, int index,
                      const std::string &directory = DIRECTORY);

std::string partPath(const std::string &name, int part,
                     const std::string &directory = DIRECTORY);

std::string musicPath(const std::string &name,
                      const std::string &directory = DIRECTORY);

std::string samplePath(const std::string &name, int sample,
                       const std::string &directory = DIRECTORY);

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_ASSETS_H_
