#ifndef ENGINE_MENUTEMPO_H_
#define ENGINE_MENUTEMPO_H_

namespace openfranko {
namespace src {
namespace engine {

inline constexpr int CONVERTED_MENU_TEMPO = 37;

constexpr double menuTuneScale(int tempo) {
  return static_cast<double>(tempo) / CONVERTED_MENU_TEMPO;
}

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_MENUTEMPO_H_
