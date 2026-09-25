#ifndef ENGINE_STATES_WORLDSOFTWARESTATE_H_
#define ENGINE_STATES_WORLDSOFTWARESTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/Bitmap.h"
#include "../../../systems/Canvas.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/AmigaDisplay.h"
#include "../../effects/FotoSequence.h"
#include "../IEngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace worldSoftware {

class WorldSoftwareState : public IEngineState {
public:
  WorldSoftwareState(systems::VideoSystem &videoSystem,
                     systems::AudioSystem &audioSystem);
  ~WorldSoftwareState();

  std::optional<EngineStateEnum> update() override;

private:
  systems::VideoSystem &m_videoSystem;
  systems::AudioSystem &m_audioSystem;
  effects::VisibleRows m_rows;
  systems::IndexedBitmap m_picture;
  systems::Canvas m_screen;
  effects::FotoSequence m_sequence;
};

} // namespace worldSoftware
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_WORLDSOFTWARESTATE_H_
