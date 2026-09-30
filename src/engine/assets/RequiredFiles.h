#ifndef ENGINE_ASSETS_REQUIREDFILES_H_
#define ENGINE_ASSETS_REQUIREDFILES_H_

#include "../GameVersion.h"
#include "Assets.h"
#include "Files.h"

#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

std::vector<std::string>
requiredFiles(GameVersion version, const std::string &directory = DIRECTORY);

std::vector<std::string> missingFiles(const Files &files, GameVersion version,
                                      const std::string &directory = DIRECTORY);

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_REQUIREDFILES_H_
