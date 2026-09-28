#include "CheatCodes.h"

#include <array>
#include <cstdint>

namespace openfranko::src::engine::street {
namespace {

constexpr int RG = 6;
constexpr int RO = 14;

constexpr int KEY_SHIFT = 4;

struct RegisterCheat {
  const char *word;
  int16_t value;
};

constexpr std::array<RegisterCheat, 3> LIVES = {{
    {"TAVRIA", 10000},
    {"MUTANT", 15},
    {"DOMAN", 9},
}};

constexpr std::array<RegisterCheat, 3> VERSION12_LIVES = {{
    {"TAVRIA", 10000},
    {"MUTANT", 12},
    {"CEAT", 6},
}};

constexpr std::array<RegisterCheat, 2> STAGES = {{
    {"CENT", 1},
    {"DRZE", 2},
}};

constexpr const char *SHORT_LEVELS = "SKIP";
constexpr const char *BRUTALITY = "MORAL";

char shifted(char key) { return static_cast<char>(key - KEY_SHIFT); }

bool contains(const std::string &text, const char *word) {
  std::string encoded;
  for (const char *letter = word; *letter != '\0'; ++letter) {
    encoded += shifted(*letter);
  }
  return text.find(encoded) != std::string::npos;
}

} // namespace

void typeCheatKey(std::string &text, char key) {
  const char upper =
      key >= 'a' && key <= 'z' ? static_cast<char>(key - 'a' + 'A') : key;
  text += shifted(upper);
  if (text.size() > CHEAT_TEXT_LENGTH) {
    text.erase(0, text.size() - CHEAT_TEXT_LENGTH);
  }
}

void applyCheatCodes(GameSession &session) {
  const std::string &text = session.textBuffer;
  const bool version12 = session.version == GameVersion::V12;
  for (const RegisterCheat &cheat : version12 ? VERSION12_LIVES : LIVES) {
    if (contains(text, cheat.word)) {
      session.registers[RG] = cheat.value;
    }
  }
  session.shortLevels = contains(text, SHORT_LEVELS);
  for (const RegisterCheat &cheat : STAGES) {
    if (contains(text, cheat.word)) {
      session.registers[RO] = cheat.value;
    }
  }
  if (!version12 && contains(text, BRUTALITY)) {
    session.brutality = true;
  }
}

} // namespace openfranko::src::engine::street
