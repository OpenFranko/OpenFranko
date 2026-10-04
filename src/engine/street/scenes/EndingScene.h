#ifndef ENGINE_STREET_SCENES_ENDINGSCENE_H_
#define ENGINE_STREET_SCENES_ENDINGSCENE_H_

#include "../../../systems/graphics/Display.h"
#include "../../amal/Machine.h"
#include "../../effects/color/AmigaPalette.h"
#include "../../effects/color/PaletteFader.h"
#include "../core/Bobs.h"
#include "../core/DoubleBuffer.h"
#include "../core/EndingCredits.h"
#include "../core/IndexedSurface.h"
#include "../core/SurfacePair.h"
#include "../core/UpdateHold.h"
#include "../session/GameSession.h"
#include "../ui/LoadingQueue.h"
#include "../ui/StageFrame.h"
#include "../ui/StatusPanel.h"
#include "StreetHost.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class EndingScene {
public:
  static constexpr int SCREEN_WIDTH = 320;
  static constexpr int SCREEN_HEIGHT = 256;
  static constexpr int DISPLAY_LINE = 50;
  static constexpr int FILES = 4;
  static constexpr int SECOND_DANCE_PAGE = 10;

  EndingScene(StreetHost &host, session::GameSession &session,
              bool ntsc = false);

  void showSprites(bool stills, bool dancers);
  void advance(int16_t joystick);
  void compose(std::vector<uint32_t> &frame) const;
  systems::graphics::Display output() const;
  systems::graphics::Display upcomingOutput() const;
  void output(systems::graphics::Display &display) const;
  void upcomingOutput(systems::graphics::Display &display) const;

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
  const core::IndexedSurface &preparedPage() const;
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

  struct PageGlyph {
    int x = 0;
    int y = 0;
    int image = 0;
  };

  Flow wait(int frames, Step next);
  Flow hold(int frames, Step next);
  void buildOutput(systems::graphics::Display &display, bool upcoming) const;
  void stageFrame();
  bool kliker(int16_t joystick, int frames);
  void runBasic(int16_t joystick);
  void preparePage();
  void start();
  void era();
  void stepCredits();
  void finishCredits();
  void fotoWhite();
  void hideStill();
  void farewell();
  void dance();
  void secondDance();
  void textScreen();
  void pageUp();
  Flow musicFade();
  void off();
  void openScreen(int number, int top, int height,
                  effects::color::AmigaPalette palette);
  void closeScreen(int number);
  void redraw();
  void stillTest();

  StreetHost &m_host;
  session::GameSession &m_session;
  amal::Machine m_machine;
  ui::LoadingQueue m_loading;
  core::ImageBank m_images;
  core::ImageBank m_parked;
  core::BobLayer m_bobs;
  std::array<Screen, 2> m_screens;
  core::SurfacePair m_display;
  core::BobLayer m_stillBobs;
  std::vector<core::Sprite> m_stillSprites;
  bool m_sprites = false;
  bool m_dancerSprites = false;
  bool m_stillSprited = false;
  bool m_stillVbl = false;
  std::optional<core::DoubleBuffer> m_dancerBuffer;
  int m_bobScreen = 0;
  std::optional<session::BossExit> m_stage;
  std::unique_ptr<ui::StatusPanel> m_panel;
  bool m_stageShown = false;
  bool m_panelShown = false;
  int m_panelTop = ui::PANEL_DISPLAY_Y;
  core::Picture m_picture;
  effects::color::AmigaPalette m_picturePalette;
  effects::color::PaletteFader m_fader;
  effects::color::AmigaColor m_border = ui::STAGE_BORDER;
  bool m_dancerCopper = false;
  core::EndingCredits m_credits;
  std::unique_ptr<StreetHost::CreditsLoad> m_creditsLoad;

  Step m_step = Step::Start;
  int m_frame = 0;
  int m_resumeFrame = 0;
  core::UpdateHold m_hold;
  int m_count = 0;
  int m_page = 0;
  core::IndexedSurface m_nextPage = core::IndexedSurface(0, 0);
  std::vector<PageGlyph> m_pageGlyphs;
  std::size_t m_pastedGlyphs = 0;
  int m_preparedPage = -1;
  bool m_pagePasted = false;
  bool m_ntsc = false;
  int m_displayLine = DISPLAY_LINE;
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SCENES_ENDINGSCENE_H_
