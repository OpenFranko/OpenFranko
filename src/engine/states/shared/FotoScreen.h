#ifndef ENGINE_STATES_SHARED_FOTOSCREEN_H_
#define ENGINE_STATES_SHARED_FOTOSCREEN_H_

#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/Monitor.h"
#include "../../AmigaDisplay.h"
#include "../../assets/Files.h"
#include "../../effects/sequences/FotoSequence.h"

#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

class FotoScreen {
public:
  FotoScreen(systems::graphics::Monitor &monitor, assets::Files &files,
             const std::string &picturePath, int displayLine,
             const effects::sequences::FotoSequence::Timings &timings);

  effects::sequences::FotoSequence &sequence();
  void shake();
  void advance();

private:
  int raisedLines() const;

  systems::graphics::Monitor &m_monitor;
  VisibleRows m_rows;
  systems::graphics::IndexedBitmap m_picture;
  systems::graphics::Canvas m_screen;
  effects::sequences::FotoSequence m_sequence;
  bool m_shaking = false;
};

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SHARED_FOTOSCREEN_H_
