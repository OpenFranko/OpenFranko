#ifndef ENGINE_STREET_GAMEOVERSCENE_H_
#define ENGINE_STREET_GAMEOVERSCENE_H_

#include "../effects/AmalAnim.h"
#include "../effects/AmigaPalette.h"
#include "../effects/PaletteFader.h"
#include "Bobs.h"
#include "DoubleBuffer.h"
#include "IndexedSurface.h"
#include "LoadingMock.h"
#include "StreetStage.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

class GameOverScene {
public:
  static constexpr int WIDTH = 368;
  static constexpr int HEIGHT = 256;
  static constexpr int PICTURE_WIDTH = 1008;
  static constexpr int PICTURE_HEIGHT = 256;
  static constexpr int DISPLAY_LINE = 45;
  static constexpr int PAN_END = 680;
  static constexpr int FILES = 3;

  explicit GameOverScene(StreetHost &host);

  void advance(int16_t joystick);
  void compose(std::vector<uint32_t> &frame) const;

  bool isShown() const;
  bool isPanning() const;
  bool isFinished() const;
  int offset() const;
  const BobLayer &bobs() const;
  const effects::AmigaPalette &palette() const;

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
  LoadingMock m_loading;
  ImageBank m_images;
  BobLayer m_bobs;
  Picture m_picture;
  IndexedSurface m_screen;
  std::optional<DoubleBuffer> m_buffer;
  effects::AmigaPalette m_palette;
  effects::AmigaPalette m_rainbow;
  effects::PaletteFader m_fader;
  effects::AmalAnim m_hand;

  Step m_step = Step::Close;
  effects::AmigaColor m_border;
  bool m_shown = false;
  bool m_rainbowShown = false;
  bool m_animating = false;
  int m_offset = 0;
  int m_shownOffset = 0;
  int m_count = 0;
  int m_frame = 0;
  int m_resumeFrame = 0;
  int m_holdStart = -1;
  int m_holdUntil = -1;
  int m_shownFrom = 0;
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_GAMEOVERSCENE_H_
