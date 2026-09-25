#ifndef ENGINE_EFFECTS_MENUSEQUENCE_H_
#define ENGINE_EFFECTS_MENUSEQUENCE_H_

#include "AmalMotion.h"
#include "AmigaPalette.h"
#include "CreditScroll.h"
#include "GameOptions.h"
#include "InkeyBuffer.h"
#include "PaletteFader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

class MenuSequence {
public:
  struct Joystick {
    bool up = false;
    bool down = false;
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

  static constexpr std::size_t BOBS = 10;
  static constexpr int ATTRACT_AFTER = 300;

  MenuSequence(GameOptions &options, AmigaPalette palette,
               InkeyBuffer &keyboard);

  void setMouseButton(bool down);
  void advance(const Joystick &joystick);
  void resumeAfterAttract();

  const std::array<Bob, BOBS> &bobs() const;
  const std::array<Bob, BOBS> &shownBobs() const;
  const AmigaPalette &palette() const;
  const std::string &keysRead() const;
  bool isAttractDue() const;
  bool isScreenShown() const;
  bool isFinished() const;

private:
  enum class Phase { Unpacking, Opening, Choosing, Leaving, Closing, Finished };
  enum class Resume { Nothing, Hand, Choosing, Leaving };

  void runScript(const Joystick &joystick);
  void choose(const Joystick &joystick);
  void finishPass();
  void readKeys();
  void moveHand(const Joystick &joystick);
  void activate();
  void flyIcons(std::size_t row, bool in);
  void startCredits();
  void runAmal();
  void placeHand();
  Bob &bob(int number);

  GameOptions &m_options;
  AmigaPalette m_palette;
  PaletteFader m_fader;
  std::array<Bob, BOBS> m_bobs{};
  std::array<Bob, BOBS> m_shownBobs{};
  std::array<AmalMotion, BOBS> m_motions{};
  std::array<std::optional<CreditScroll>, 3> m_credits{};
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
  bool m_attractDue = false;
  bool m_mouseButton = false;
  bool m_screenShown = false;
  bool m_busy = false;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_MENUSEQUENCE_H_
