#ifndef ENGINE_STATES_PROTECTIONCHECK_PROTECTIONCHECKSTATE_H_
#define ENGINE_STATES_PROTECTIONCHECK_PROTECTIONCHECKSTATE_H_

#include "../../../systems/audio/Speaker.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/Monitor.h"
#include "../../AmigaDisplay.h"
#include "../../InkeyBuffer.h"
#include "../../assets/Files.h"
#include "../../effects/color/PaletteFlasher.h"
#include "../../effects/protection/CodeCardCheck.h"
#include "../EngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace protectionCheck {

class ProtectionCheckState : public EngineState {
public:
  enum class Check { Title, Stage3 };

  ProtectionCheckState(systems::graphics::Monitor &monitor,
                       systems::audio::Speaker &speaker, assets::Files &files,
                       InkeyBuffer &keyboard, Check check = Check::Title);

  std::optional<EngineStateId> update() override;
  bool isEnteringText() const override;

  const effects::protection::CodeCardCheck &check() const;

private:
  enum class Step { Unpack, Ask, Hidden, Closed, FailureUnpacked, Hang };

  std::optional<EngineStateId> runCheck();
  bool takeAnswer();
  void showQuestion();
  void showFailure();
  void draw();
  void show();

  systems::graphics::Monitor &m_monitor;
  systems::audio::Speaker &m_speaker;
  assets::Files &m_files;
  InkeyBuffer &m_keyboard;
  Check m_kind;
  effects::protection::CodeCardCheck m_check;
  int m_loadingFrames;
  Step m_step = Step::Unpack;
  int m_frame = 0;
  int m_resumeFrame = SCREEN_OPEN_VBLS;
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

#endif // ENGINE_STATES_PROTECTIONCHECK_PROTECTIONCHECKSTATE_H_
