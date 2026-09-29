#ifndef ENGINE_STREET_SCENES_HIGHSCORESCENE_H_
#define ENGINE_STREET_SCENES_HIGHSCORESCENE_H_

#include "../../../systems/graphics/Display.h"
#include "../../GameOptions.h"
#include "../../effects/color/AmigaPalette.h"
#include "../../effects/color/PaletteFader.h"
#include "../core/Bobs.h"
#include "../core/HighScoreTable.h"
#include "../core/IndexedSurface.h"
#include "../session/GameSession.h"
#include "../ui/LoadingQueue.h"
#include "StreetHost.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class HighScoreScene {
public:
  enum class Outcome { Running, Menu, Continue };

  static constexpr int SCREEN_WIDTH = 320;
  static constexpr int SCREEN_HEIGHT = 256;
  static constexpr int DISPLAY_LINE = 50;
  static constexpr int FILES = 4;
  static constexpr char BACKSPACE = '\b';
  static constexpr char RETURN = '\r';

  using Save = std::function<void(const core::HighScoreTable &)>;

  static void pasteRow(const core::HighScoreTable &table, int row,
                       const core::Paste &paste);

  HighScoreScene(StreetHost &host, session::GameSession &session,
                 const GameOptions &options, Save save);

  void advance();
  void compose(std::vector<uint32_t> &frame) const;
  systems::graphics::Display output() const;

  Outcome outcome() const;
  bool isShown() const;
  bool isEntering() const;
  int slot() const;
  int rowsShown() const;
  const std::string &name() const;
  const core::BobLayer &bobs() const;
  const core::IndexedSurface &screen() const;
  const effects::color::AmigaPalette &palette() const;

private:
  enum class Step {
    Reset,
    Loading,
    MenuMusic,
    Pictures,
    Loaded,
    Dim,
    Dimmed,
    Relight,
    Row,
    EntryStart,
    Entry,
    Hold,
    FadeOut,
    Clear,
    Finished
  };
  enum class Flow { Continue, Yield };

  Flow wait(int frames, Step next);
  void runBasic();
  void reset();
  void queuePictures();
  Flow loaded();
  Flow dim();
  Flow dimmed();
  void relight();
  Flow row();
  void startEntry();
  Flow entry();
  void restoreCell();
  void commit();
  void clear();
  void redraw();

  StreetHost &m_host;
  session::GameSession &m_session;
  const GameOptions &m_options;
  Save m_save;
  ui::LoadingQueue m_loading;
  core::ImageBank m_images;
  core::BobLayer m_bobs;
  core::Picture m_picture;
  effects::color::AmigaPalette m_picturePalette;
  core::IndexedSurface m_screen;
  core::IndexedSurface m_scratch;
  core::IndexedSurface m_display;
  effects::color::AmigaPalette m_palette;
  effects::color::PaletteFader m_fader;

  Step m_step = Step::Reset;
  Step m_afterLoading = Step::Reset;
  Outcome m_outcome = Outcome::Running;
  int m_frame = 0;
  int m_resumeFrame = 0;
  bool m_shown = false;
  bool m_entering = false;
  int m_round = 0;
  int m_row = 0;
  int m_rowsShown = 0;
  int m_slot = core::HighScoreTable::NO_SLOT;
  int m_x = 0;
  int m_y = 0;
  std::string m_name;
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SCENES_HIGHSCORESCENE_H_
