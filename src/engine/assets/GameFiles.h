#ifndef ENGINE_ASSETS_GAMEFILES_H_
#define ENGINE_ASSETS_GAMEFILES_H_

#include "Files.h"

#include <memory>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

std::unique_ptr<Files> openGameFiles();

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_GAMEFILES_H_
