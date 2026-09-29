#ifndef ENGINE_STREET_HIGHSCORESCENE_H_
#define ENGINE_STREET_HIGHSCORESCENE_H_

#include "../../../systems/graphics/Display.h"
#include "../../effects/color/AmigaPalette.h"
#include "../../effects/color/PaletteFader.h"
#include "../../effects/core/GameOptions.h"
#include "../core/Bobs.h"
#include "../core/IndexedSurface.h"
#include "../core/LoadingMock.h"
#include "../ui/GameSession.h"
#include "HighScoreTable.h"
#include "StreetStage.h"

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

  static constexpr int WIDTH = 320;
  static constexpr int HEIGHT = 256;
  static constexpr int DISPLAY_LINE = 50;
  static constexpr int FILES = 4;
  static constexpr char BACKSPACE = '\b';
  static constexpr char RETURN = '\r';

  using Save = std::function<void(const HighScoreTable &)>;

  HighScoreScene(StreetHost &host, ui::GameSession &session,
                 const effects::core::GameOptions &options, Save save);

  void advance();
  void compose(std::vector<uint32_t> &frame) const;
  systems::Display output() const;

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
  void pasteRow(int row);
  void startEntry();
  Flow entry();
  void restoreCell();
  void commit();
  void clear();
  void redraw();

  StreetHost &m_host;
  ui::GameSession &m_session;
  const effects::core::GameOptions &m_options;
  Save m_save;
  core::LoadingMock m_loading;
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
  int m_slot = HighScoreTable::NO_SLOT;
  int m_x = 0;
  int m_y = 0;
  std::string m_name;
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_HIGHSCORESCENE_H_
