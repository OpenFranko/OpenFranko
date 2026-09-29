#ifndef ENGINE_EFFECTS_SEQUENCES_ATTRACTSEQUENCE_H_
#define ENGINE_EFFECTS_SEQUENCES_ATTRACTSEQUENCE_H_

#include "../../AmigaDisplay.h"
#include "../color/AmigaPalette.h"

#include <array>
#include <cstddef>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {
namespace sequences {

class AttractSequence {
public:
  enum class Kind { Title, Hiscores };

  static constexpr int HISCORE_ROWS = 10;

  struct Relit {
    std::size_t index = 0;
    color::AmigaColor color = 0;
  };

  static constexpr std::array<Relit, 3> RELIT = {{
      {29, 0x769},
      {30, 0xB95},
      {31, 0xFC0},
  }};

  AttractSequence(Kind kind, color::AmigaPalette picturePalette);

  void advance(bool joystickTouched);

  Kind kind() const;
  bool isShowing() const;
  const color::AmigaPalette &palette() const;
  int rowsShown() const;
  bool isWaiting() const;
  bool isFinished() const;

private:
  void advanceTitle(bool joystickTouched);
  void advanceHiscores(bool joystickTouched);

  Kind m_kind;
  color::AmigaPalette m_palette;
  int m_frame = -UNPACK_VBLS;
  int m_rows = 0;
  bool m_showing = false;
  bool m_waiting = false;
  bool m_finished = false;
};

} // namespace sequences
} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_SEQUENCES_ATTRACTSEQUENCE_H_
