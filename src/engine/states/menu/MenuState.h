#ifndef ENGINE_STATES_MENU_MENUSTATE_H_
#define ENGINE_STATES_MENU_MENUSTATE_H_

#include "../../../systems/audio/Speaker.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/Monitor.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../GameOptions.h"
#include "../../assets/Files.h"
#include "../../effects/sequences/AttractSequence.h"
#include "../../effects/sequences/MenuSequence.h"
#include "../../street/session/GameSession.h"
#include "../EngineState.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace menu {

class MenuState : public EngineState {
public:
  MenuState(systems::graphics::Monitor &monitor,
            systems::audio::Speaker &speaker,
            systems::input::ControllerSystem &controllerSystem,
            assets::Files &files, GameOptions &options,
            street::session::GameSession &session);

  std::optional<EngineStateId> update() override;

  static std::vector<std::string> menuPaths(GameVersion version);
  static std::vector<std::string> attractPaths(GameVersion version);
  static std::string tunePath(GameVersion version);

private:
  enum class AttractLoad { Title, Hiscores, Letters, Done };

  void loadAttractStep();
  void loadAttract();
  bool loadAttractPicture(int resource,
                          systems::graphics::IndexedBitmap &picture);
  void
  advanceAttract(const effects::sequences::MenuSequence::Joystick &joystick);
  void switchStandard();
  void startAttract();
  void drawMenu();
  bool menuSprites();
  const systems::graphics::IndexedBitmap &mirroredBob(std::size_t index);
  void drawAttract();
  void drawAttractPicture();
  void drawHiscoreRow(int row);
  void show(const systems::graphics::Canvas &screen);

  systems::graphics::Monitor &m_monitor;
  systems::audio::Speaker &m_speaker;
  systems::input::ControllerSystem &m_controllerSystem;
  assets::Files &m_files;
  GameOptions &m_options;
  street::session::GameSession &m_session;
  systems::graphics::IndexedBitmap m_backdrop;
  systems::graphics::IndexedBitmap m_title;
  systems::graphics::IndexedBitmap m_hiscores;
  std::vector<systems::graphics::IndexedBitmap> m_menuBobs;
  std::vector<systems::graphics::IndexedBitmap> m_mirroredBobs;
  std::vector<systems::graphics::Sprite> m_sprites;
  std::vector<systems::graphics::IndexedBitmap> m_letters;
  systems::graphics::Canvas m_menuScreen;
  systems::graphics::Canvas m_attractScreen;
  effects::sequences::MenuSequence m_menu;
  effects::color::AmigaPalette m_titlePalette;
  effects::color::AmigaPalette m_hiscorePalette;
  AttractLoad m_attractLoad = AttractLoad::Title;
  std::unique_ptr<assets::Files::BitmapLoad> m_pictureLoad;
  std::optional<effects::sequences::AttractSequence> m_attract;
  effects::sequences::AttractSequence::Kind m_nextAttract =
      effects::sequences::AttractSequence::Kind::Title;
  bool m_backdropShown = false;
  int m_attractTop = 0;
  int m_rowsDrawn = 0;
  int m_attractClosing = 0;
  int m_musicWait = 0;
};

} // namespace menu
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_MENU_MENUSTATE_H_
