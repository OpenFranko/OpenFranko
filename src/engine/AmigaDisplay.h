#ifndef ENGINE_AMIGADISPLAY_H_
#define ENGINE_AMIGADISPLAY_H_

#include <algorithm>

namespace openfranko {
namespace src {
namespace engine {

inline constexpr int FIRST_VISIBLE_LINE = 26;
inline constexpr int LAST_PAL_LINE = 309;
inline constexpr int LAST_NTSC_LINE = 261;

constexpr int lastVisibleLine(bool ntsc) {
  return ntsc ? LAST_NTSC_LINE : LAST_PAL_LINE;
}

inline constexpr int SCREEN_OPEN_VBLS = 1;
inline constexpr int SCREEN_CLOSE_VBLS = 4;
inline constexpr int SCREEN_CLOSE_SHOWN_VBLS = 2;
inline constexpr int SCREEN_CLOSE_HIDDEN_VBLS =
    SCREEN_CLOSE_VBLS - SCREEN_CLOSE_SHOWN_VBLS;
inline constexpr int SCREEN_REOPEN_VBLS = SCREEN_CLOSE_VBLS + SCREEN_OPEN_VBLS;
inline constexpr int DOUBLE_BUFFER_VBLS = 3;
inline constexpr int AUTOBACK_VBLS = 3;
inline constexpr int UNPACK_VBLS = 1;

inline constexpr int NTSC_PICTURE_RAISE = 27;

constexpr int pictureLine(int palLine, bool ntsc) {
  return palLine - (ntsc ? NTSC_PICTURE_RAISE : 0);
}

struct VisibleRows {
  int first = 0;
  int count = 0;
};

constexpr VisibleRows visibleRows(int displayY, int height, bool ntsc) {
  const int first = std::max(0, FIRST_VISIBLE_LINE - displayY);
  const int last = std::min(height - 1, lastVisibleLine(ntsc) - displayY);
  return {first, std::max(0, last - first + 1)};
}

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_AMIGADISPLAY_H_
