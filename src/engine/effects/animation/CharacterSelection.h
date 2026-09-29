#ifndef ENGINE_EFFECTS_ANIMATION_CHARACTERSELECTION_H_
#define ENGINE_EFFECTS_ANIMATION_CHARACTERSELECTION_H_

#include "../../GameVersion.h"
#include "../core/GameOptions.h"
#include "AmalAnim.h"
#include "AmalMotion.h"

#include <cstdint>
#include <optional>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {
namespace animation {

class CharacterSelection {
public:
  struct Joystick {
    bool left = false;
    bool right = false;
    bool fire = false;
  };

  struct Bob {
    bool shown = false;
    int16_t x = 0;
    int16_t y = 0;
    int image = 0;
    bool flipped = false;
  };

  explicit CharacterSelection(core::GameOptions &options, int otherScreens = 0,
                              GameVersion version = GameVersion::V10);

  void advance(const Joystick &joystick);

  const Bob &hand() const;
  const Bob &face() const;
  std::optional<int> sample() const;
  std::optional<int> musicVolume() const;
  bool stopsMusic() const;
  bool isScreenShown() const;
  bool isFinished() const;

private:
  void choose(core::Character character);
  void runScript(int time);
  void close(int time);

  core::GameOptions &m_options;
  GameVersion m_version;
  Bob m_hand;
  Bob m_face;
  AmalMotion m_handMotion;
  AmalAnim m_faceAnim;
  std::optional<int> m_confirmedAt;
  int m_frame = 0;
  std::optional<int> m_sample;
  std::optional<int> m_musicVolume;
  bool m_stopsMusic = false;
  bool m_screenShown = false;
  bool m_finished = false;
  int m_otherScreens = 0;
  std::optional<int> m_closedAt;
};

} // namespace animation
} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_ANIMATION_CHARACTERSELECTION_H_
