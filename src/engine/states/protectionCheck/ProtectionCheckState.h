#ifndef ENGINE_STATES_PROTECTIONCHECKSTATE_H_
#define ENGINE_STATES_PROTECTIONCHECKSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/Bitmap.h"
#include "../../../systems/Canvas.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/AmigaDisplay.h"
#include "../../effects/CodeCardCheck.h"
#include "../../effects/InkeyBuffer.h"
#include "../../effects/PaletteFlasher.h"
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
                       effects::InkeyBuffer &keyboard,
                       Check check = Check::Title);

  std::optional<EngineStateEnum> update() override;

  const effects::CodeCardCheck &check() const;

private:
  enum class Step { Unpack, Ask, Hidden, Closed, FailureUnpacked, Hang };

  std::optional<EngineStateEnum> runCheck();
  bool takeAnswer();
  void showQuestion();
  void showFailure();
  void draw();
  void show();

  systems::VideoSystem &m_videoSystem;
  systems::AudioSystem &m_audioSystem;
  effects::InkeyBuffer &m_keyboard;
  Check m_kind;
  effects::CodeCardCheck m_check;
  int m_loadingFrames;
  Step m_step = Step::Unpack;
  int m_frame = 0;
  int m_resumeFrame;
  bool m_questionShown = false;
  bool m_failureShown = false;
  systems::Canvas m_screen;
  systems::IndexedBitmap m_question;
  systems::IndexedBitmap m_failure;
  effects::AmigaPalette m_questionPalette;
  effects::AmigaColor m_border;
  effects::PaletteFlasher m_flasher;
  int m_failureTop = 0;
};

} // namespace protectionCheck
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_PROTECTIONCHECKSTATE_H_
