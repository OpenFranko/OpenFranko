#ifndef ENGINE_STREET_ENDINGSCENE_H_
#define ENGINE_STREET_ENDINGSCENE_H_

#include "../../../systems/graphics/Display.h"
#include "../../amal/Machine.h"
#include "../../effects/color/AmigaPalette.h"
#include "../../effects/color/PaletteFader.h"
#include "../core/Bobs.h"
#include "../core/DoubleBuffer.h"
#include "../core/IndexedSurface.h"
#include "../core/LoadingMock.h"
#include "../ui/GameSession.h"
#include "../ui/StatusPanel.h"
#include "EndingCredits.h"
#include "StreetStage.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

class EndingScene {
public:
  static constexpr int WIDTH = 320;
  static constexpr int HEIGHT = 256;
  static constexpr int DISPLAY_LINE = 50;
  static constexpr int FILES = 4;
  static constexpr int SECOND_DANCE_PAGE = 10;

  EndingScene(StreetHost &host, GameSession &session, bool ntsc = false);

  void advance(int16_t joystick);
  void compose(std::vector<uint32_t> &frame) const;
  systems::Display output() const;

  bool isLoading() const;
  bool isShowingStill() const;
  bool isWalkingAway() const;
  bool isShowingCredits() const;
  bool isFinished() const;
  int page() const;
  bool isShown(int screen) const;
  bool isStageShown() const;
  int displayLine() const;
  effects::color::AmigaColor border() const;
  const core::IndexedSurface &screen(int number) const;
  const effects::color::AmigaPalette &palette(int number) const;
  const core::BobLayer &bobs() const;
  const core::IndexedSurface *panel() const;
  amal::Machine &machine();

private:
  enum class Step {
    Start,
    Era,
    Loading,
    StageGone,
    ClosePanel,
    PanelGone,
    Foto,
    FotoWhite,
    FotoFade,
    FotoClose,
    Still,
    StillHidden,
    StillKliker,
    StillOff,
    Grey,
    Farewell,
    WalkAway,
    FarewellKliker,
    CloseStill,
    StillGone,
    CloseHidden,
    Dancer,
    DancerShown,
    Dance,
    TextScreen,
    Page,
    PageFade,
    PageClear,
    FinalKliker,
    TextGone,
    CloseDancer,
    DancerGone,
    MusicFade,
    Finished
  };
  enum class Flow { Continue, Yield };

  struct Screen {
    bool open = false;
    bool hidden = false;
    int top = 0;
    core::IndexedSurface surface = core::IndexedSurface(0, 0);
    effects::color::AmigaPalette palette;
  };

  Flow wait(int frames, Step next);
  Flow hold(int frames, Step next);
  bool holdsAtStart() const;
  bool holdsAtEnd() const;
  void stageFrame();
  bool kliker(int16_t joystick, int frames);
  void runBasic(int16_t joystick);
  void start();
  void era();
  void fotoWhite();
  void hideStill();
  void farewell();
  void dance();
  void secondDance();
  void textScreen();
  void font(const std::string &text, int y);
  void pageUp();
  Flow musicFade();
  void off();
  void openScreen(int number, int top, int height,
                  effects::color::AmigaPalette palette);
  void closeScreen(int number);
  void redraw();
  void stillTest();

  StreetHost &m_host;
  GameSession &m_session;
  amal::Machine m_machine;
  core::LoadingMock m_loading;
  core::ImageBank m_images;
  core::ImageBank m_parked;
  core::BobLayer m_bobs;
  std::array<Screen, 2> m_screens;
  core::IndexedSurface m_display;
  core::BobLayer m_stillBobs;
  bool m_stillVbl = false;
  std::optional<core::DoubleBuffer> m_dancerBuffer;
  int m_bobScreen = 0;
  std::optional<BossExit> m_stage;
  std::unique_ptr<StatusPanel> m_panel;
  bool m_stageShown = false;
  bool m_panelShown = false;
  int m_panelTop = core::PANEL_DISPLAY_Y;
  core::Picture m_picture;
  effects::color::AmigaPalette m_picturePalette;
  effects::color::PaletteFader m_fader;
  effects::color::AmigaColor m_border;
  bool m_dancerCopper = false;
  EndingCredits m_credits;

  Step m_step = Step::Start;
  int m_frame = 0;
  int m_resumeFrame = 0;
  int m_holdStart = -1;
  int m_holdUntil = -1;
  int m_count = 0;
  int m_page = 0;
  bool m_ntsc = false;
  int m_displayLine = DISPLAY_LINE;
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_ENDINGSCENE_H_
