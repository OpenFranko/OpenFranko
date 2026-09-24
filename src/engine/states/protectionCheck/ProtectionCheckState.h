#ifndef ENGINE_STATES_PROTECTIONCHECKSTATE_H_
#define ENGINE_STATES_PROTECTIONCHECKSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/CodeCardCheck.h"
#include "../IEngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace protectionCheck {

class ProtectionCheckState : public IEngineState {
public:
  enum class Check { Title, Stage3 };

  ProtectionCheckState(systems::VideoSystem &videoSystem,
                       systems::AudioSystem &audioSystem,
                       systems::ControllerSystem &controllerSystem,
                       Check check = Check::Title);
  ~ProtectionCheckState();

  std::optional<EngineStateEnum> update() override;

  const effects::CodeCardCheck &check() const;

private:
  void showQuestion();
  void showFailure();

  systems::VideoSystem &m_videoSystem;
  systems::AudioSystem &m_audioSystem;
  systems::ControllerSystem &m_controllerSystem;
  Check m_kind;
  effects::CodeCardCheck m_check;
  int m_loadingFrames;
};

} // namespace protectionCheck
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_PROTECTIONCHECKSTATE_H_
