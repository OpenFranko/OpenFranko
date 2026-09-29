#ifndef ENGINE_STATES_CHARACTERSELECTION_CHARACTERSELECTIONSTATE_H_
#define ENGINE_STATES_CHARACTERSELECTION_CHARACTERSELECTIONSTATE_H_

#include "../../../systems/audio/Speaker.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/Monitor.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../AmigaDisplay.h"
#include "../../GameOptions.h"
#include "../../assets/Files.h"
#include "../../effects/sequences/CharacterSelectionSequence.h"
#include "../../street/session/GameSession.h"
#include "../EngineState.h"

#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace characterSelection {

class CharacterSelectionState : public EngineState {
public:
  CharacterSelectionState(systems::graphics::Monitor &monitor,
                          systems::audio::Speaker &speaker,
                          systems::input::ControllerSystem &controllerSystem,
                          assets::Files &files, GameOptions &options,
                          street::session::GameSession &session);
  ~CharacterSelectionState() override;

  std::optional<EngineStateId> update() override;

private:
  void draw();
  EngineStateId firstStreet() const;

  systems::graphics::Monitor &m_monitor;
  systems::audio::Speaker &m_speaker;
  systems::input::ControllerSystem &m_controllerSystem;
  street::session::GameSession &m_session;
  effects::sequences::CharacterSelectionSequence m_selection;
  VisibleRows m_rows;
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

#endif // ENGINE_STATES_CHARACTERSELECTION_CHARACTERSELECTIONSTATE_H_
