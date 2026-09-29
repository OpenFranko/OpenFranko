#ifndef ENGINE_STATES_SHARED_FOTOSCREEN_H_
#define ENGINE_STATES_SHARED_FOTOSCREEN_H_

#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../AmigaDisplay.h"
#include "../../effects/sequences/FotoSequence.h"

#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

class FotoScreen {
public:
  FotoScreen(systems::graphics::VideoSystem &videoSystem,
             const std::string &picturePath, int displayLine,
             const effects::sequences::FotoSequence::Timings &timings);

  effects::sequences::FotoSequence &sequence();
  void advance();

private:
  systems::graphics::VideoSystem &m_videoSystem;
  VisibleRows m_rows;
  systems::graphics::IndexedBitmap m_picture;
  systems::graphics::Canvas m_screen;
  effects::sequences::FotoSequence m_sequence;
};

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SHARED_FOTOSCREEN_H_
