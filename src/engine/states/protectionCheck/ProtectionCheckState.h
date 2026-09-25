#ifndef ENGINE_STATES_PROTECTIONCHECKSTATE_H_
#define ENGINE_STATES_PROTECTIONCHECKSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/AmigaDisplay.h"
#include "../../effects/CodeCardCheck.h"
#include "../IEngineState.h"

#include <deque>

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
  enum class Step { Unpack, Ask, Closed, FailureUnpacked, Hang };

  std::optional<EngineStateEnum> runCheck();
  bool takeAnswer();
  void showQuestion();
  void showFailure();
  void draw();

  systems::VideoSystem &m_videoSystem;
  systems::AudioSystem &m_audioSystem;
  systems::ControllerSystem &m_controllerSystem;
  Check m_kind;
  effects::CodeCardCheck m_check;
  int m_loadingFrames;
  std::deque<char> m_typed;
  Step m_step = Step::Unpack;
  int m_frame = 0;
  int m_resumeFrame;
  bool m_questionShown = false;
  bool m_failureShown = false;
  int m_failureTop = 0;
};

} // namespace protectionCheck
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_PROTECTIONCHECKSTATE_H_
