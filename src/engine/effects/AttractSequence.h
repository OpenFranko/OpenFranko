#ifndef ENGINE_EFFECTS_ATTRACTSEQUENCE_H_
#define ENGINE_EFFECTS_ATTRACTSEQUENCE_H_

#include "AmigaPalette.h"

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

class AttractSequence {
public:
  enum class Kind { Title, Hiscores };

  static constexpr int HISCORE_ROWS = 10;

  AttractSequence(Kind kind, AmigaPalette picturePalette);

  void advance(bool joystickTouched);

  Kind kind() const;
  bool isShowing() const;
  const AmigaPalette &palette() const;
  int rowsShown() const;
  bool isFinished() const;

private:
  void advanceTitle(bool joystickTouched);
  void advanceHiscores(bool joystickTouched);

  Kind m_kind;
  AmigaPalette m_palette;
  int m_frame = 0;
  int m_rows = 0;
  bool m_showing = false;
  bool m_finished = false;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_ATTRACTSEQUENCE_H_
