#ifndef ENGINE_EFFECTS_AMIGADISPLAY_H_
#define ENGINE_EFFECTS_AMIGADISPLAY_H_

#include <algorithm>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

constexpr int FIRST_VISIBLE_LINE = 26;
constexpr int LAST_PAL_LINE = 309;
constexpr int LAST_NTSC_LINE = 261;

constexpr int lastVisibleLine(bool ntsc) {
  return ntsc ? LAST_NTSC_LINE : LAST_PAL_LINE;
}

constexpr int SCREEN_OPEN_VBLS = 1;
constexpr int SCREEN_CLOSE_VBLS = 4;
constexpr int SCREEN_CLOSE_SHOWN_VBLS = 2;
constexpr int SCREEN_CLOSE_HIDDEN_VBLS =
    SCREEN_CLOSE_VBLS - SCREEN_CLOSE_SHOWN_VBLS;
constexpr int SCREEN_REOPEN_VBLS = SCREEN_CLOSE_VBLS + SCREEN_OPEN_VBLS;

constexpr int NTSC_PICTURE_RAISE = 27;

constexpr int pictureLine(int palLine, bool ntsc) {
  return palLine - (ntsc ? NTSC_PICTURE_RAISE : 0);
}

struct VisibleRows {
  int first;
  int count;
};

constexpr VisibleRows visibleRows(int displayY, int height, bool ntsc) {
  const int first = std::max(0, FIRST_VISIBLE_LINE - displayY);
  const int last = std::min(height - 1, lastVisibleLine(ntsc) - displayY);
  return {first, std::max(0, last - first + 1)};
}

constexpr int CONVERTED_MENU_TEMPO = 37;
constexpr int NTSC_TEMPO_DROP = 5;

constexpr int menuTempo(bool ntsc) {
  return CONVERTED_MENU_TEMPO - (ntsc ? NTSC_TEMPO_DROP : 0);
}

constexpr double menuTuneScale(int tempo) {
  return static_cast<double>(tempo) / CONVERTED_MENU_TEMPO;
}

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_AMIGADISPLAY_H_
