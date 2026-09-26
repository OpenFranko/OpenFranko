#ifndef ENGINE_STATES_MENUSTATE_H_
#define ENGINE_STATES_MENUSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/Bitmap.h"
#include "../../../systems/Canvas.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/AttractSequence.h"
#include "../../effects/GameOptions.h"
#include "../../effects/MenuSequence.h"
#include "../../street/GameSession.h"
#include "../IEngineState.h"

#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace menu {

class MenuState : public IEngineState {
public:
  MenuState(systems::VideoSystem &videoSystem,
            systems::AudioSystem &audioSystem,
            systems::ControllerSystem &controllerSystem,
            effects::GameOptions &options, street::GameSession &session);

  std::optional<EngineStateEnum> update() override;

private:
  void advanceAttract(const effects::MenuSequence::Joystick &joystick);
  void switchStandard();
  void startAttract();
  void drawMenu();
  void drawAttract();
  void drawAttractPicture();
  void drawHiscoreRow(int row);
  void show(const systems::Canvas &screen);

  systems::VideoSystem &m_videoSystem;
  systems::AudioSystem &m_audioSystem;
  systems::ControllerSystem &m_controllerSystem;
  effects::GameOptions &m_options;
  street::GameSession &m_session;
  systems::IndexedBitmap m_backdrop;
  systems::IndexedBitmap m_title;
  systems::IndexedBitmap m_hiscores;
  std::vector<systems::IndexedBitmap> m_menuBobs;
  std::vector<systems::IndexedBitmap> m_letters;
  systems::Canvas m_menuScreen;
  systems::Canvas m_attractScreen;
  effects::MenuSequence m_menu;
  effects::AmigaPalette m_titlePalette;
  effects::AmigaPalette m_hiscorePalette;
  std::optional<effects::AttractSequence> m_attract;
  effects::AttractSequence::Kind m_nextAttract =
      effects::AttractSequence::Kind::Title;
  int m_attractTop = 0;
  int m_attractClosing = 0;
  int m_musicWait = 0;
};

} // namespace menu
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_MENUSTATE_H_
