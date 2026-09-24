#ifndef ENGINE_EFFECTS_AMIGADISPLAY_H_
#define ENGINE_EFFECTS_AMIGADISPLAY_H_

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
