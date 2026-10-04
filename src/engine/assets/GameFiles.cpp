#include "GameFiles.h"

#include "ArchiveFiles.h"
#include "Assets.h"
#include "DiskFiles.h"

#include <filesystem>

namespace openfranko::src::engine::assets {

std::unique_ptr<Files> openGameFiles() {
  if (std::filesystem::exists(ARCHIVE)) {
    return std::make_unique<ArchiveFiles>(ARCHIVE);
  }
  return std::make_unique<DiskFiles>();
}

} // namespace openfranko::src::engine::assets
