#ifndef ENGINE_EFFECTS_SEQUENCES_MENUSEQUENCE_H_
#define ENGINE_EFFECTS_SEQUENCES_MENUSEQUENCE_H_

#include "../../GameOptions.h"
#include "../../GameVersion.h"
#include "../../InkeyBuffer.h"
#include "../animation/AmalMotion.h"
#include "../animation/Bob.h"
#include "../animation/CreditScroll.h"
#include "../color/AmigaPalette.h"
#include "../color/PaletteFader.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {
namespace sequences {

class MenuSequence {
public:
  struct Joystick {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool fire = false;
  };

  static constexpr std::size_t BOBS = 10;
  static constexpr int ATTRACT_AFTER = 300;

  MenuSequence(GameOptions &options, color::AmigaPalette palette,
               InkeyBuffer &keyboard, GameVersion version = GameVersion::V10);

  void setMouseButton(bool down);
  void advance(const Joystick &joystick);
  void resumeAfterAttract();

  const std::array<animation::Bob, BOBS> &bobs() const;
  const std::array<animation::Bob, BOBS> &shownBobs() const;
  const color::AmigaPalette &palette() const;
  const std::string &keysRead() const;
  bool isAttractDue() const;
  bool isScreenShown() const;
  bool isFinished() const;

private:
  enum class Phase { Unpacking, Opening, Choosing, Leaving, Closing, Finished };
  enum class Resume { Nothing, Hand, Choosing, Leaving };

  void runScript(const Joystick &joystick);
  void choose(const Joystick &joystick);
  void chooseInTurn(const Joystick &joystick);
  void finishPass();
  void readKeys();
  void moveHand(const Joystick &joystick);
  void activate();
  void flyIcons(std::size_t row, bool in);
  void startCredits();
  void runAmal();
  void placeHand();
  animation::Bob &bob(int number);

  GameOptions &m_options;
  GameVersion m_version;
  color::AmigaPalette m_palette;
  color::PaletteFader m_fader;
  std::array<animation::Bob, BOBS> m_bobs{};
  std::array<animation::Bob, BOBS> m_shownBobs{};
  std::array<animation::AmalMotion, BOBS> m_motions{};
  std::array<std::optional<animation::CreditScroll>, 3> m_credits{};
  InkeyBuffer &m_keyboard;
  std::string m_keysRead;
  Phase m_phase = Phase::Unpacking;
  Resume m_resume = Resume::Nothing;
  int m_frame = 0;
  int m_phaseStart = 0;
  int m_resumeFrame = 0;
  int m_timer = 0;
  int m_column = 0;
  int m_row = 0;
  int m_direction = 0;
  bool m_attractDue = false;
  bool m_mouseButton = false;
  bool m_screenShown = false;
  bool m_busy = false;
};

} // namespace sequences
} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_SEQUENCES_MENUSEQUENCE_H_
