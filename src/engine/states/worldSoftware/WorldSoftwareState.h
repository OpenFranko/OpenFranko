#ifndef ENGINE_STATES_WORLDSOFTWARESTATE_H_
#define ENGINE_STATES_WORLDSOFTWARESTATE_H_

#include "../../../systems/audio/AudioSystem.h"
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
namespace worldSoftware {

class WorldSoftwareState : public IEngineState {
public:
  WorldSoftwareState(systems::graphics::VideoSystem &videoSystem,
                     systems::audio::AudioSystem &audioSystem);
  ~WorldSoftwareState();

  std::optional<EngineStateEnum> update() override;

private:
  systems::graphics::VideoSystem &m_videoSystem;
  systems::audio::AudioSystem &m_audioSystem;
  effects::color::VisibleRows m_rows;
  systems::graphics::IndexedBitmap m_picture;
  systems::graphics::Canvas m_screen;
  effects::sequences::FotoSequence m_sequence;
};

} // namespace worldSoftware
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_WORLDSOFTWARESTATE_H_
