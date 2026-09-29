#ifndef ENGINE_EFFECTS_SEQUENCES_CHARACTERSELECTIONSEQUENCE_H_
#define ENGINE_EFFECTS_SEQUENCES_CHARACTERSELECTIONSEQUENCE_H_

#include "../../GameOptions.h"
#include "../../GameVersion.h"
#include "../animation/AmalAnim.h"
#include "../animation/AmalMotion.h"

#include <cstdint>
#include <optional>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {
namespace sequences {

class CharacterSelectionSequence {
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

  explicit CharacterSelectionSequence(GameOptions &options,
                                      int otherScreens = 0,
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
  void choose(Character character);
  void runScript(int time);
  void close(int time);

  GameOptions &m_options;
  GameVersion m_version;
  Bob m_hand;
  Bob m_face;
  animation::AmalMotion m_handMotion;
  animation::AmalAnim m_faceAnim;
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

} // namespace sequences
} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_SEQUENCES_CHARACTERSELECTIONSEQUENCE_H_
