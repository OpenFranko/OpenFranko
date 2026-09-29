#ifndef ENGINE_STATES_MIRAGE_MIRAGESTATE_H_
#define ENGINE_STATES_MIRAGE_MIRAGESTATE_H_

#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../effects/color/AmigaDisplay.h"
#include "../../effects/sequences/FotoSequence.h"
#include "../IEngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace mirage {

class MirageState : public IEngineState {
public:
  explicit MirageState(systems::graphics::VideoSystem &videoSystem);

  std::optional<EngineStateEnum> update() override;

private:
  systems::graphics::VideoSystem &m_videoSystem;
  effects::color::VisibleRows m_rows;
  systems::graphics::IndexedBitmap m_picture;
  systems::graphics::Canvas m_screen;
  effects::sequences::FotoSequence m_sequence;
};

} // namespace mirage
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_MIRAGE_MIRAGESTATE_H_
