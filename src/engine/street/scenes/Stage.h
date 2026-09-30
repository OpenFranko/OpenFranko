#ifndef ENGINE_STREET_SCENES_STAGE_H_
#define ENGINE_STREET_SCENES_STAGE_H_

#include "../../GameOptions.h"
#include "../../amal/Machine.h"
#include "../../effects/color/AmigaPalette.h"
#include "../core/Bobs.h"
#include "../core/DoubleBuffer.h"
#include "../core/IndexedSurface.h"
#include "../core/UpdateHold.h"
#include "../session/GameSession.h"
#include "../ui/LoadingQueue.h"
#include "../ui/StageFrame.h"
#include "../ui/StatusPanel.h"
#include "StreetHost.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class Stage {
public:
  enum class Outcome { Playing, Cleared, GameOver, Quit };

  static constexpr int SCREEN_WIDTH = 320;
  static constexpr int SCREEN_HEIGHT = 222;

  virtual ~Stage() = default;

  void advance(const StreetInput &input);
  void compose(std::vector<uint32_t> &frame) const;
  systems::graphics::Display output() const;

  Outcome outcome() const;
  const core::BobLayer &bobs() const;
  const core::IndexedSurface &screen() const;
  const core::IndexedSurface &display() const;
  const ui::StatusPanel *panel() const;
  amal::Machine &machine();
  bool isScreenShown() const;
  bool isPanelShown() const;

protected:
  enum class Flow { Continue, Yield };

  static constexpr int PLAYER = 1;
  static constexpr int INDICATOR = 12;
  static constexpr int HIDDEN_IMAGE = 10;
  static constexpr int SPLAT_IMAGE = 9;
  static constexpr int IDLE_IMAGE = 17;
  static constexpr int SCREEN_SHAKE_CHANNEL = 0;
  static constexpr int INDICATOR_CHANNEL = 13;
  static constexpr int ENEMY_BLOOD_CHANNEL = 14;
  static constexpr int PLAYER_BLOOD_CHANNEL = 15;
  static constexpr int PRIORITY_VOICE = 1;
  static constexpr int STREET_Y = 172;

  Stage(StreetHost &host, session::GameSession &session, GameOptions &options);

  virtual void runBasic(const StreetInput &input) = 0;

  static int16_t word(int value);

  int16_t &global(int index);
  int16_t &reg(int channel, int index);
  int xBob(int number) const;
  int yBob(int number) const;
  int iBob(int number) const;
  bool col(int number) const;
  int stage() const;
  ui::StatusPanel::Stats stats() const;
  void openPanel();
  void playMusic();
  void updatePanel();
  void stall();
  void autoback(core::DoubleBuffer::Op op);
  bool pasteStalled(int x, int y, int image);
  void putBlock(const core::ScreenBlock &block);
  void scrollStep();
  Flow endOfPass() const;
  Flow hold(int frames);
  void test();
  ui::StageCopper registers() const;
  void hideScreen();
  void openBlankScreens();
  void gameOver();
  Flow advanceGameOver();
  bool quitsToHighScores() const;
  void sys();

  StreetHost &m_host;
  session::GameSession &m_session;
  GameOptions &m_options;
  amal::Machine m_machine;
  core::ImageBank m_images;
  core::BobLayer m_bobs;
  core::IndexedSurface m_screen;
  core::DoubleBuffer m_buffer;
  std::unique_ptr<ui::StatusPanel> m_panel;
  amal::Object m_screenDisplay;
  ui::StageDisplay m_copper;
  effects::color::AmigaPalette m_palette;
  effects::color::AmigaPalette m_panelPalette;
  ui::LoadingQueue m_loading;

  Outcome m_outcome = Outcome::Playing;
  long m_frame = 0;
  long m_resumeFrame = 0;
  long m_passFrame = 0;
  int m_energyShown = 0;
  int m_killsShown = 0;
  int m_screenOffsetX = 0;
  bool m_screenShown = true;
  bool m_panelShown = true;
  bool m_escape = false;
  bool m_mouseButton = false;
  bool m_musicLoaded = true;
  bool m_holdsWhileClosing = false;

private:
  enum class Closing {
    Wait,
    Cleared,
    ScreenGone,
    PanelClose,
    PanelGone,
    Closed
  };

  Flow closePlayScreen();
  Flow closeWait(int frames);

  Closing m_closing = Closing::Wait;
  core::UpdateHold m_hold;
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SCENES_STAGE_H_
