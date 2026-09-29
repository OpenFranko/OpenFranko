#ifndef ENGINE_STREET_SCENES_GAMEOVERSCENE_H_
#define ENGINE_STREET_SCENES_GAMEOVERSCENE_H_

#include "../../../systems/graphics/Display.h"
#include "../../effects/animation/AmalAnim.h"
#include "../../effects/color/AmigaPalette.h"
#include "../../effects/color/PaletteFader.h"
#include "../core/Bobs.h"
#include "../core/DoubleBuffer.h"
#include "../core/IndexedSurface.h"
#include "../ui/LoadingMock.h"
#include "StreetStage.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class GameOverScene {
public:
  static constexpr int WIDTH = 368;
  static constexpr int HEIGHT = 256;
  static constexpr int PICTURE_WIDTH = 1008;
  static constexpr int PICTURE_HEIGHT = 256;
  static constexpr int DISPLAY_LINE = 45;
  static constexpr int PAN_END = 680;
  static constexpr int FILES = 3;

  GameOverScene(StreetHost &host, ui::GameSession &session);

  void advance(int16_t joystick);
  void compose(std::vector<uint32_t> &frame) const;
  systems::graphics::Display output() const;

  bool isShown() const;
  bool isPanning() const;
  bool isFinished() const;
  int offset() const;
  const core::BobLayer &bobs() const;
  const effects::color::AmigaPalette &palette() const;

private:
  enum class Step {
    Close,
    Loading,
    Open,
    Unpacked,
    Opened,
    Pan,
    Click,
    MusicFade,
    Hold,
    CloseShown,
    Closed,
    Finished
  };
  enum class Flow { Continue, Yield };

  Flow wait(int frames, Step next);
  bool holdsAtStart() const;
  bool holdsAtEnd() const;
  void close();
  void unpack();
  void open();
  Flow pan();
  Flow click(int16_t joystick);
  Flow musicFade();
  Flow hold();
  void closeGraveyard();

  StreetHost &m_host;
  ui::GameSession &m_session;
  ui::LoadingMock m_loading;
  core::ImageBank m_images;
  core::BobLayer m_bobs;
  core::Picture m_picture;
  core::IndexedSurface m_screen;
  std::optional<core::DoubleBuffer> m_buffer;
  effects::color::AmigaPalette m_palette;
  effects::color::AmigaPalette m_rainbow;
  effects::color::PaletteFader m_fader;
  effects::animation::AmalAnim m_hand;

  Step m_step = Step::Close;
  effects::color::AmigaColor m_border;
  effects::color::AmigaColor m_copperBorder;
  bool m_shown = false;
  bool m_copperShown = false;
  bool m_rainbowShown = false;
  bool m_animating = false;
  int m_offset = 0;
  int m_copperOffset = 0;
  int m_shownOffset = 0;
  int m_count = 0;
  int m_frame = 0;
  int m_resumeFrame = 0;
  int m_holdStart = -1;
  int m_holdUntil = -1;
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SCENES_GAMEOVERSCENE_H_
