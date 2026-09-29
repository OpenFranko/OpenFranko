#ifndef ENGINE_STATES_MENUSTATE_H_
#define ENGINE_STATES_MENUSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../effects/core/GameOptions.h"
#include "../../effects/sequences/AttractSequence.h"
#include "../../effects/sequences/MenuSequence.h"
#include "../../street/ui/GameSession.h"
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
  MenuState(systems::graphics::VideoSystem &videoSystem,
            systems::audio::AudioSystem &audioSystem,
            systems::input::ControllerSystem &controllerSystem,
            effects::core::GameOptions &options,
            street::ui::GameSession &session);

  std::optional<EngineStateEnum> update() override;

private:
  void
  advanceAttract(const effects::sequences::MenuSequence::Joystick &joystick);
  void switchStandard();
  void startAttract();
  void drawMenu();
  void drawAttract();
  void drawAttractPicture();
  void drawHiscoreRow(int row);
  void show(const systems::graphics::Canvas &screen);

  systems::graphics::VideoSystem &m_videoSystem;
  systems::audio::AudioSystem &m_audioSystem;
  systems::input::ControllerSystem &m_controllerSystem;
  effects::core::GameOptions &m_options;
  street::ui::GameSession &m_session;
  systems::graphics::IndexedBitmap m_backdrop;
  systems::graphics::IndexedBitmap m_title;
  systems::graphics::IndexedBitmap m_hiscores;
  std::vector<systems::graphics::IndexedBitmap> m_menuBobs;
  std::vector<systems::graphics::IndexedBitmap> m_letters;
  systems::graphics::Canvas m_menuScreen;
  systems::graphics::Canvas m_attractScreen;
  effects::sequences::MenuSequence m_menu;
  effects::color::AmigaPalette m_titlePalette;
  effects::color::AmigaPalette m_hiscorePalette;
  std::optional<effects::sequences::AttractSequence> m_attract;
  effects::sequences::AttractSequence::Kind m_nextAttract =
      effects::sequences::AttractSequence::Kind::Title;
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
