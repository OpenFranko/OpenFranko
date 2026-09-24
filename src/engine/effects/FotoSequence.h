#ifndef ENGINE_EFFECTS_FOTOSEQUENCE_H_
#define ENGINE_EFFECTS_FOTOSEQUENCE_H_

#include "PaletteFader.h"
#include "PaletteFlasher.h"

#include <cstddef>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

class FotoSequence {
public:
  struct Timings {
    int fadeInSpeed;
    int holdFrames;
    int fadeOutSpeed;
    int fadeOutFrames;
  };

  FotoSequence(AmigaPalette picturePalette, Timings timings);

  bool advance();

  void flash(std::size_t color, FlashSteps steps);

  const AmigaPalette &palette() const;
  bool isFinished() const;

  int frame() const;
  int holdStart() const;

private:
  int fadeInStart() const;
  int fadeOutStart() const;
  int totalFrames() const;

  AmigaPalette m_picturePalette;
  AmigaPalette m_palette;
  Timings m_timings;
  PaletteFader m_fader;
  PaletteFlasher m_flasher;
  int m_frame = 0;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_FOTOSEQUENCE_H_
