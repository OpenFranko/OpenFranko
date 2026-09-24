#ifndef ENGINE_STREET_HIGHSCORESCENE_H_
#define ENGINE_STREET_HIGHSCORESCENE_H_

#include "../effects/AmigaPalette.h"
#include "../effects/GameOptions.h"
#include "../effects/PaletteFader.h"
#include "Bobs.h"
#include "GameSession.h"
#include "HighScoreTable.h"
#include "IndexedSurface.h"
#include "LoadingMock.h"
#include "StreetStage.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

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

  HighScoreScene(StreetHost &host, GameSession &session,
                 const effects::GameOptions &options, Save save);

  void advance(char key);
  void compose(std::vector<uint32_t> &frame) const;

  Outcome outcome() const;
  bool isShown() const;
  bool isEntering() const;
  int slot() const;
  int rowsShown() const;
  const std::string &name() const;
  const BobLayer &bobs() const;
  const IndexedSurface &screen() const;
  const effects::AmigaPalette &palette() const;

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
  void runBasic(char key);
  void reset();
  void queuePictures();
  Flow loaded();
  Flow dim();
  Flow dimmed();
  void relight();
  Flow row();
  void pasteRow(int row);
  void startEntry();
  Flow entry(char key);
  void restoreCell();
  void commit();
  void clear();
  void redraw();

  StreetHost &m_host;
  GameSession &m_session;
  const effects::GameOptions &m_options;
  Save m_save;
  LoadingMock m_loading;
  ImageBank m_images;
  BobLayer m_bobs;
  Picture m_picture;
  effects::AmigaPalette m_picturePalette;
  IndexedSurface m_screen;
  IndexedSurface m_scratch;
  IndexedSurface m_display;
  effects::AmigaPalette m_palette;
  effects::PaletteFader m_fader;

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

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_HIGHSCORESCENE_H_
