#ifndef ENGINE_STATES_CHARACTERSELECTIONSTATE_H_
#define ENGINE_STATES_CHARACTERSELECTIONSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../effects/animation/CharacterSelection.h"
#include "../../effects/color/AmigaDisplay.h"
#include "../../effects/core/GameOptions.h"
#include "../../street/ui/GameSession.h"
#include "../IEngineState.h"

#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace characterSelection {

class CharacterSelectionState : public IEngineState {
public:
  CharacterSelectionState(systems::graphics::VideoSystem &videoSystem,
                          systems::audio::AudioSystem &audioSystem,
                          systems::input::ControllerSystem &controllerSystem,
                          effects::core::GameOptions &options,
                          street::ui::GameSession &session);
  ~CharacterSelectionState();

  std::optional<EngineStateEnum> update() override;

private:
  void draw();
  EngineStateEnum firstStreet() const;

  systems::graphics::VideoSystem &m_videoSystem;
  systems::audio::AudioSystem &m_audioSystem;
  systems::input::ControllerSystem &m_controllerSystem;
  street::ui::GameSession &m_session;
  effects::animation::CharacterSelection m_selection;
  effects::color::VisibleRows m_rows;
  systems::graphics::IndexedBitmap m_picture;
  effects::color::AmigaPalette m_screenPalette;
  std::vector<systems::graphics::IndexedBitmap> m_sprites;
  systems::graphics::Canvas m_screen;
};

} // namespace characterSelection
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_CHARACTERSELECTIONSTATE_H_
