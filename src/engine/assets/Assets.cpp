#include "Assets.h"

#include <cstdio>
#include <filesystem>
#include <stdexcept>

namespace openfranko::src::engine::assets {
namespace {

constexpr auto VERSION12_MARKER = "p0/p0.bmp";

constexpr int LAST_SPRITE_SET = 0xFF;
constexpr int FIRST_TILES = 0x137;
constexpr int LAST_TILES = 0x154;
constexpr int TILES_BASE = 300;
constexpr int FIRST_MUSIC = 0x259;
constexpr int LAST_MUSIC = 0x262;
constexpr int MUSIC_BASE = 0x258;
constexpr int MISSING_MUSIC = 0x260;
constexpr int FIRST_STAGE_DATA = 0x384;
constexpr int LAST_STAGE_DATA = 0x38C;
constexpr int FIRST_SCREEN = 0x3B7;
constexpr int LAST_SCREEN = 0x3C2;
constexpr int FIRST_SCREEN_NUMBER = 51;
constexpr int MIRAGE_LOGO = 0x3C3;
constexpr int WORLD_SOFTWARE_LOGO = 50;

std::string hexName(int resource) {
  char name[8];
  std::snprintf(name, sizeof(name), "%04X", resource);
  return name;
}

std::string version12Name(int resource) {
  if (resource >= 0 && resource <= LAST_SPRITE_SET) {
    return "s" + std::to_string(resource);
  }
  if (resource >= FIRST_TILES && resource <= LAST_TILES) {
    return "t" + std::to_string(resource - TILES_BASE);
  }
  if (resource >= FIRST_MUSIC && resource <= LAST_MUSIC &&
      resource != MISSING_MUSIC) {
    return "m" + std::to_string(resource - MUSIC_BASE);
  }
  if (resource >= FIRST_STAGE_DATA && resource <= LAST_STAGE_DATA) {
    return "p" + std::to_string(resource - FIRST_STAGE_DATA);
  }
  if (resource >= FIRST_SCREEN && resource <= LAST_SCREEN) {
    return "p" + std::to_string(resource - FIRST_SCREEN + FIRST_SCREEN_NUMBER);
  }
  if (resource == MIRAGE_LOGO) {
    return "p" + std::to_string(WORLD_SOFTWARE_LOGO);
  }
  throw std::runtime_error("Resource " + hexName(resource) +
                           " has no Franko 1.2 file");
}

} // namespace

GameVersion detectVersion(const std::string &directory) {
  return std::filesystem::exists(std::filesystem::path(directory) /
                                 VERSION12_MARKER)
             ? GameVersion::V12
             : GameVersion::V10;
}

std::string resourceName(int resource, GameVersion version) {
  return version == GameVersion::V12 ? version12Name(resource)
                                     : hexName(resource);
}

std::string picturePath(const std::string &name, const std::string &directory) {
  return directory + "/" + name + ".bmp";
}

std::string imagePath(const std::string &name, int index,
                      const std::string &directory) {
  char file[32];
  std::snprintf(file, sizeof(file), "_%03d.bmp", index);
  return directory + "/" + name + "/" + name + file;
}

std::string partPath(const std::string &name, int part,
                     const std::string &directory) {
  const std::string file =
      part == 0 ? name + ".bmp" : name + "_" + std::to_string(part) + ".bmp";
  return directory + "/" + name + "/" + file;
}

std::string musicPath(const std::string &name, const std::string &directory) {
  return directory + "/" + name + ".s3m";
}

std::string samplePath(const std::string &name, int sample,
                       const std::string &directory) {
  const std::string prefix = name + "_sam" + std::to_string(sample) + "_";
  std::error_code error;
  for (const auto &entry : std::filesystem::directory_iterator(
           std::filesystem::path(directory) / name, error)) {
    const std::string file = entry.path().filename().string();
    if (file.compare(0, prefix.size(), prefix) == 0 &&
        entry.path().extension() == ".wav") {
      return entry.path().string();
    }
  }
  return {};
}

} // namespace openfranko::src::engine::assets
