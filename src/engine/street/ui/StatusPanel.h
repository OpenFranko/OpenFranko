#ifndef ENGINE_STREET_STATUSPANEL_H_
#define ENGINE_STREET_STATUSPANEL_H_

#include "../../GameVersion.h"
#include "../core/IndexedSurface.h"

namespace openfranko {
namespace src {
namespace engine {
namespace street {

class StatusPanel {
public:
  static constexpr int WIDTH = 304;
  static constexpr int HEIGHT = 48;
  static constexpr int VISIBLE_HEIGHT = 32;

  struct Stats {
    int energy = 0;
    int stage = 0;
    int kills = 0;
    int lives = 0;
  };

  StatusPanel(core::Picture loadingStrip, core::Picture artwork,
              GameVersion version = GameVersion::V10);

  void showLoading();
  void showWaiting();
  void score(const Stats &stats);
  void drawKills(int kills);
  void loseEnergy(int energy);
  void gainEnergy(int energy);

  const core::IndexedSurface &surface() const;

private:
  core::Picture m_loadingStrip;
  core::Picture m_artwork;
  int m_livesDigitX;
  core::IndexedSurface m_surface;
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_STATUSPANEL_H_
