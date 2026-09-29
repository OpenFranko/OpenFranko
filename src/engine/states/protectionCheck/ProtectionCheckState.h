#ifndef ENGINE_STATES_PROTECTIONCHECKSTATE_H_
#define ENGINE_STATES_PROTECTIONCHECKSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../effects/color/AmigaDisplay.h"
#include "../../effects/color/PaletteFlasher.h"
#include "../../effects/core/InkeyBuffer.h"
#include "../../effects/sequences/CodeCardCheck.h"
#include "../IEngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace protectionCheck {

class ProtectionCheckState : public IEngineState {
public:
  enum class Check { Title, Stage3 };

  ProtectionCheckState(systems::graphics::VideoSystem &videoSystem,
                       systems::audio::AudioSystem &audioSystem,
                       effects::core::InkeyBuffer &keyboard,
                       Check check = Check::Title);

  std::optional<EngineStateEnum> update() override;

  const effects::sequences::CodeCardCheck &check() const;

private:
  enum class Step { Unpack, Ask, Hidden, Closed, FailureUnpacked, Hang };

  std::optional<EngineStateEnum> runCheck();
  bool takeAnswer();
  void showQuestion();
  void showFailure();
  void draw();
  void show();

  systems::graphics::VideoSystem &m_videoSystem;
  systems::audio::AudioSystem &m_audioSystem;
  effects::core::InkeyBuffer &m_keyboard;
  Check m_kind;
  effects::sequences::CodeCardCheck m_check;
  int m_loadingFrames;
  Step m_step = Step::Unpack;
  int m_frame = 0;
  int m_resumeFrame;
  bool m_questionShown = false;
  bool m_failureShown = false;
  systems::graphics::Canvas m_screen;
  systems::graphics::IndexedBitmap m_question;
  systems::graphics::IndexedBitmap m_failure;
  effects::color::AmigaPalette m_questionPalette;
  effects::color::AmigaColor m_border;
  effects::color::PaletteFlasher m_flasher;
  int m_failureTop = 0;
};

} // namespace protectionCheck
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_PROTECTIONCHECKSTATE_H_
