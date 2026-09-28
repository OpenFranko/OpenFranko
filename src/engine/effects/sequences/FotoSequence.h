#ifndef ENGINE_EFFECTS_FOTOSEQUENCE_H_
#define ENGINE_EFFECTS_FOTOSEQUENCE_H_

#include "../color/PaletteFader.h"
#include "../color/PaletteFlasher.h"

#include <cstddef>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {
namespace sequences {

class FotoSequence {
public:
  struct Timings {
    int fadeInSpeed;
    int holdFrames;
    int fadeOutSpeed;
    int fadeOutFrames;
    bool replacesScreen;
  };

  FotoSequence(color::AmigaPalette picturePalette, Timings timings);

  bool advance();

  void flash(std::size_t color, color::FlashSteps steps);

  const color::AmigaPalette &palette() const;
  bool isShown() const;
  bool isFinished() const;

  int frame() const;
  int holdStart() const;

private:
  int whiteStart() const;
  int fadeInStart() const;
  int fadeOutStart() const;
  int closeStart() const;
  int totalFrames() const;

  color::AmigaPalette m_picturePalette;
  color::AmigaPalette m_palette;
  Timings m_timings;
  color::PaletteFader m_fader;
  color::PaletteFlasher m_flasher;
  int m_frame = 0;
};

} // namespace sequences
} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_FOTOSEQUENCE_H_
