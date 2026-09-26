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
inline constexpr std::string_view CEMETERY_PICTURE = "03BB";

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

namespace version12 {

namespace fileIds {

inline constexpr std::string_view WORLD_SOFTWARE_PALETTE = "s50";
inline constexpr std::string_view WORLD_SOFTWARE_LOGO = "p50";

} // namespace fileIds

enum class Loader { Data, Data16, Bobs, Music, Coded, Stage };

struct File {
  std::string_view name;
  Loader loader;
  std::string_view counterpart;
};

inline constexpr std::array<File, 104> FILES = {{
    {"m1", Loader::Music, "0259"},   {"m2", Loader::Music, "025A"},
    {"m3", Loader::Music, "025B"},   {"m4", Loader::Music, "025C"},
    {"m5", Loader::Music, "025D"},   {"m6", Loader::Music, "025E"},
    {"m7", Loader::Music, "025F"},   {"m9", Loader::Music, "0261"},
    {"m10", Loader::Music, "0262"},  {"m11", Loader::Music, ""},
    {"p0", Loader::Data, "0384"},    {"p1", Loader::Data16, "0385"},
    {"p2", Loader::Data16, "0386"},  {"p3", Loader::Data16, "0387"},
    {"p4", Loader::Data16, "0388"},  {"p5", Loader::Data16, "0389"},
    {"p6", Loader::Data16, "038A"},  {"p7", Loader::Data16, "038B"},
    {"p8", Loader::Data16, "038C"},  {"p50", Loader::Data, "03C3"},
    {"p51", Loader::Data, "03B7"},   {"p52", Loader::Coded, "03B8"},
    {"p53", Loader::Data16, "03B9"}, {"p54", Loader::Data16, "03BA"},
    {"p55", Loader::Data16, "03BB"}, {"p56", Loader::Data16, "03BC"},
    {"p57", Loader::Data16, "03BD"}, {"p58", Loader::Data, "03BE"},
    {"p59", Loader::Data, "03BF"},   {"p60", Loader::Data, "03C0"},
    {"p61", Loader::Data16, "03C1"}, {"p62", Loader::Data16, "03C2"},
    {"p80", Loader::Data, ""},       {"p81", Loader::Data, ""},
    {"p82", Loader::Data, ""},       {"p83", Loader::Data, ""},
    {"p84", Loader::Data, ""},       {"p85", Loader::Data, ""},
    {"s0", Loader::Bobs, "0000"},    {"s1", Loader::Bobs, "0001"},
    {"s2", Loader::Bobs, "0002"},    {"s3", Loader::Bobs, "0003"},
    {"s4", Loader::Bobs, "0004"},    {"s5", Loader::Bobs, "0005"},
    {"s6", Loader::Bobs, "0006"},    {"s7", Loader::Bobs, "0007"},
    {"s8", Loader::Bobs, "0008"},    {"s9", Loader::Bobs, "0009"},
    {"s10", Loader::Bobs, "000A"},   {"s11", Loader::Bobs, "000B"},
    {"s12", Loader::Bobs, "000C"},   {"s13", Loader::Bobs, "000D"},
    {"s14", Loader::Bobs, "000E"},   {"s15", Loader::Bobs, "000F"},
    {"s16", Loader::Bobs, "0010"},   {"s17", Loader::Bobs, "0011"},
    {"s18", Loader::Bobs, "0012"},   {"s19", Loader::Bobs, "0013"},
    {"s20", Loader::Bobs, "0014"},   {"s21", Loader::Bobs, "0015"},
    {"s50", Loader::Bobs, ""},       {"s52", Loader::Bobs, "0034"},
    {"s53", Loader::Bobs, "0035"},   {"s54", Loader::Bobs, "0036"},
    {"s55", Loader::Bobs, "0037"},   {"s56", Loader::Bobs, "0038"},
    {"s148", Loader::Bobs, "0094"},  {"s149", Loader::Bobs, "0095"},
    {"s150", Loader::Bobs, "0096"},  {"s198", Loader::Bobs, "00C6"},
    {"s199", Loader::Bobs, "00C7"},  {"s200", Loader::Bobs, "00C8"},
    {"s246", Loader::Bobs, "00F6"},  {"s247", Loader::Bobs, "00F7"},
    {"s248", Loader::Bobs, "00F8"},  {"s249", Loader::Bobs, "00F9"},
    {"s250", Loader::Bobs, "00FA"},  {"s251", Loader::Bobs, "00FB"},
    {"s252", Loader::Bobs, "00FC"},  {"s253", Loader::Bobs, "00FD"},
    {"s254", Loader::Bobs, "00FE"},  {"s255", Loader::Bobs, "00FF"},
    {"t11", Loader::Stage, "0137"},  {"t12", Loader::Stage, "0138"},
    {"t13", Loader::Stage, "0139"},  {"t14", Loader::Stage, "013A"},
    {"t15", Loader::Stage, "013B"},  {"t16", Loader::Stage, "013C"},
    {"t17", Loader::Stage, "013D"},  {"t18", Loader::Stage, "013E"},
    {"t19", Loader::Stage, "013F"},  {"t20", Loader::Stage, "0140"},
    {"t21", Loader::Stage, "0141"},  {"t22", Loader::Stage, "0142"},
    {"t23", Loader::Stage, "0143"},  {"t24", Loader::Stage, "0144"},
    {"t25", Loader::Stage, "0145"},  {"t30", Loader::Stage, "014A"},
    {"t31", Loader::Stage, "014B"},  {"t32", Loader::Stage, "014C"},
    {"t33", Loader::Stage, "014D"},  {"t34", Loader::Stage, "014E"},
    {"t35", Loader::Stage, "014F"},  {"t40", Loader::Stage, "0154"},
}};

inline constexpr const File *find(std::string_view name) {
  for (const File &file : FILES) {
    if (file.name == name) {
      return &file;
    }
  }
  return nullptr;
}

} // namespace version12

inline constexpr std::string_view version10Id(std::string_view fileId) {
  const version12::File *file = version12::find(fileId);
  return file == nullptr ? fileId : file->counterpart;
}

} // namespace gameData
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // GAMEDATA_H_
