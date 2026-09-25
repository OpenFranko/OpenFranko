#ifndef ENGINE_STATES_ENDINGSTATE_H_
#define ENGINE_STATES_ENDINGSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/AmigaDisplay.h"
#include "../../effects/GameOptions.h"
#include "../../street/EndingScene.h"
#include "../../street/GameSession.h"
#include "../IEngineState.h"
#include "../level1/EngineStreetHost.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace ending {

class EndingState : public IEngineState {
public:
  EndingState(systems::VideoSystem &videoSystem,
              systems::AudioSystem &audioSystem,
              systems::ControllerSystem &controllerSystem,
              const effects::GameOptions &options,
              street::GameSession &session);

  std::optional<EngineStateEnum> update() override;

  const street::EndingScene &scene() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  level1::EngineStreetHost m_host;
  street::EndingScene m_scene;
  effects::VisibleRows m_rows;
  std::vector<uint32_t> m_frame;
};

} // namespace ending
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_ENDINGSTATE_H_
