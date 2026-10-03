#include "Assets.h"

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string_view>

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

constexpr std::size_t IMAGE_SUFFIX_SIZE = 16;
constexpr int PADDED_LIMIT = 1000;
constexpr std::string_view WAVE = ".wav";

void appendPadded(std::string &path, int number) {
  if (number >= 0 && number < PADDED_LIMIT) {
    char hundreds = '0';
    for (; number >= 100; number -= 100) {
      ++hundreds;
    }
    char tens = '0';
    for (; number >= 10; number -= 10) {
      ++tens;
    }
    path.append(1, hundreds).append(1, tens);
    path.append(1, static_cast<char>('0' + number));
    return;
  }
  char digits[16];
  std::snprintf(digits, sizeof(digits), "%03d", number);
  path.append(digits);
}

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

GameVersion detectVersion(const Files &files, const std::string &directory) {
  return files.exists(directory + "/" + VERSION12_MARKER) ? GameVersion::V12
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
  std::string path;
  path.reserve(directory.size() + 2 * name.size() + IMAGE_SUFFIX_SIZE);
  path.append(directory).append(1, '/').append(name).append(1, '/');
  path.append(name).append(1, '_');
  appendPadded(path, index);
  return path.append(".bmp");
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

std::string samplePath(const Files &files, const std::string &name, int sample,
                       const std::string &directory) {
  const std::string prefix = name + "_sam" + std::to_string(sample) + "_";
  const std::string bank = directory + "/" + name;
  const std::unique_ptr<Files::Listing> listing = files.walk(bank);
  std::string_view file;
  while (listing->next(file)) {
    if (file.substr(0, prefix.size()) == prefix && file.size() >= WAVE.size() &&
        file.substr(file.size() - WAVE.size()) == WAVE) {
      return bank + "/" + std::string(file);
    }
  }
  return {};
}

} // namespace openfranko::src::engine::assets
