#include "RequiredFiles.h"

#include <array>
#include <cstddef>

namespace openfranko::src::engine::assets {
namespace {

enum class Kind { Frames, Parts, Picture, Music, LevelScript };

struct Resources {
  int first = 0;
  int last = 0;
  Kind kind = Kind::Picture;
};

constexpr std::array<Resources, 17> RESOURCES = {{
    {0x000, 0x015, Kind::Frames},
    {0x034, 0x038, Kind::Frames},
    {0x094, 0x096, Kind::Frames},
    {0x0C6, 0x0C8, Kind::Frames},
    {0x0F6, 0x0FF, Kind::Frames},
    {0x137, 0x145, Kind::Frames},
    {0x14A, 0x14F, Kind::Frames},
    {0x154, 0x154, Kind::Frames},
    {0x259, 0x25F, Kind::Music},
    {0x261, 0x262, Kind::Music},
    {0x384, 0x384, Kind::Parts},
    {0x385, 0x387, Kind::LevelScript},
    {0x388, 0x38C, Kind::Picture},
    {0x3B7, 0x3B7, Kind::Parts},
    {0x3B8, 0x3BD, Kind::Picture},
    {0x3BE, 0x3C0, Kind::Parts},
    {0x3C1, 0x3C3, Kind::Picture},
}};

constexpr std::array<Resources, 1> VERSION10_RESOURCES = {{
    {0x3B6, 0x3B6, Kind::Picture},
}};

constexpr std::array<Resources, 1> VERSION12_RESOURCES = {{
    {0x032, 0x032, Kind::Frames},
}};

constexpr std::array<const char *, 3> VERSION10_FILES = {
    "0263/0263_sam1_13160Hz.wav",
    "0263/0263_sam2_6453Hz.wav",
    "0384/0384_cards.bin",
};

constexpr std::array<const char *, 7> VERSION12_FILES = {
    "m11.s3m", "p80.bmp", "p81.bmp", "p82.bmp", "p83.bmp", "p84.bmp", "p85.bmp",
};

constexpr auto CREDITS_FILE = "credits.json";

std::string resourceFile(Kind kind, const std::string &name,
                         const std::string &directory) {
  switch (kind) {
  case Kind::Frames:
    return imagePath(name, 0, directory);
  case Kind::Parts:
    return partPath(name, 0, directory);
  case Kind::Picture:
    return picturePath(name, directory);
  case Kind::Music:
    return musicPath(name, directory);
  case Kind::LevelScript:
    break;
  }
  return directory + "/" + name + ".json";
}

template <std::size_t COUNT>
void addResources(const std::array<Resources, COUNT> &table,
                  GameVersion version, const std::string &directory,
                  std::vector<std::string> &paths) {
  for (const Resources &resources : table) {
    for (int resource = resources.first; resource <= resources.last;
         ++resource) {
      paths.push_back(resourceFile(resources.kind,
                                   resourceName(resource, version), directory));
    }
  }
}

template <std::size_t COUNT>
void addFiles(const std::array<const char *, COUNT> &names,
              const std::string &directory, std::vector<std::string> &paths) {
  for (const char *name : names) {
    paths.push_back(directory + "/" + name);
  }
}

} // namespace

std::vector<std::string> requiredFiles(GameVersion version,
                                       const std::string &directory) {
  std::vector<std::string> paths;
  addResources(RESOURCES, version, directory, paths);
  if (version == GameVersion::V12) {
    addResources(VERSION12_RESOURCES, version, directory, paths);
    addFiles(VERSION12_FILES, directory, paths);
  } else {
    addResources(VERSION10_RESOURCES, version, directory, paths);
    addFiles(VERSION10_FILES, directory, paths);
  }
  paths.push_back(directory + "/" + CREDITS_FILE);
  return paths;
}

std::vector<std::string> missingFiles(const Files &files, GameVersion version,
                                      const std::string &directory) {
  std::vector<std::string> missing;
  for (const std::string &path : requiredFiles(version, directory)) {
    if (!files.exists(path)) {
      missing.push_back(path);
    }
  }
  return missing;
}

} // namespace openfranko::src::engine::assets
