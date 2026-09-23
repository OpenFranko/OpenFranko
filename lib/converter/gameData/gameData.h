#ifndef GAMEDATA_H_
#define GAMEDATA_H_

#include <array>
#include <cstdint>
#include <string_view>

namespace openfranko {
namespace lib {
namespace converter {
namespace gameData {

namespace fileIds {

inline constexpr std::string_view MULTI_PALETTE_BITMAP = "0384";
inline constexpr std::string_view CODE_CARDS = "0384";
inline constexpr std::string_view SUNSET_PALETTE = "0038";
inline constexpr std::string_view STORY_PALETTE = "0037";
inline constexpr std::string_view MENU_PALETTE = "0034";
inline constexpr std::string_view MENU_35_PALETTE = "0035";
inline constexpr std::string_view CEMETERY_PALETTE = "0036";
inline constexpr std::string_view TITLE_PALETTE = "03B7";
inline constexpr std::string_view HUD_SPRITES = "03BB";

inline constexpr std::array<std::string_view, 3> PAL24_FILES = {"03BE", "03BF",
                                                                "03C0"};

inline constexpr std::array<std::string_view, 3> LEVEL_FILES = {"0385", "0386",
                                                                "0387"};

inline constexpr std::array<std::string_view, 22> TILE_FILES = {
    "0137", "0138", "0139", "013A", "013B", "013C", "013D", "013E",
    "013F", "0140", "0141", "0142", "0143", "0144", "0145", "014A",
    "0154", "014B", "014C", "014D", "014E", "014F"};

inline constexpr std::array<std::string_view, 98> EXPECTED_FILES = {
    "0000", "0001", "0002", "0003", "0004", "0005", "0006", "0007", "0008",
    "0009", "000A", "000B", "000C", "000D", "000E", "000F", "0010", "0011",
    "0012", "0013", "0014", "0015", "0034", "0035", "0036", "0037", "0038",
    "0094", "0095", "0096", "00C6", "00C7", "00C8", "00F6", "00F7", "00F8",
    "00F9", "00FA", "00FB", "00FC", "00FD", "00FE", "00FF", "0137", "0138",
    "0139", "013A", "013B", "013C", "013D", "013E", "013F", "0140", "0141",
    "0142", "0143", "0144", "0145", "014A", "014B", "014C", "014D", "014E",
    "014F", "0154", "0259", "025A", "025B", "025C", "025D", "025E", "025F",
    "0261", "0262", "0263", "0384", "0385", "0386", "0387", "0388", "0389",
    "038A", "038B", "038C", "03B6", "03B7", "03B8", "03B9", "03BA", "03BB",
    "03BC", "03BD", "03BE", "03BF", "03C0", "03C1", "03C2", "03C3"};

} // namespace fileIds

namespace resourceTypes {

inline constexpr uint16_t SPRITES = 0x0000;
inline constexpr uint16_t ICONS = 0x0200;
inline constexpr uint16_t SCREEN_PACKAGE = 0x0201;
inline constexpr uint16_t SAMPLES = 0x0300;
inline constexpr uint16_t MUSIC = 0x0400;

inline constexpr const char *name(uint16_t type) {
  switch (type) {
  case SPRITES:
    return "Sprites";
  case ICONS:
    return "Icons";
  case SCREEN_PACKAGE:
    return "ScreenPkg";
  case SAMPLES:
    return "Samples";
  case MUSIC:
    return "Music";
  default:
    return "Unknown";
  }
}

} // namespace resourceTypes

namespace audio {

inline constexpr uint16_t DEFAULT_SAMPLE_RATE = 8287;

} // namespace audio

} // namespace gameData
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // GAMEDATA_H_
