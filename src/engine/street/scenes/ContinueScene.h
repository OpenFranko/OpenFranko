#ifndef ENGINE_STREET_SCENES_CONTINUESCENE_H_
#define ENGINE_STREET_SCENES_CONTINUESCENE_H_

#include "../../../systems/graphics/Display.h"
#include "../../amal/Machine.h"
#include "../../effects/color/AmigaPalette.h"
#include "../core/Bobs.h"
#include "../core/IndexedSurface.h"
#include "../session/GameSession.h"
#include "StreetHost.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class ContinueScene {
public:
  enum class Outcome { Choosing, Continue, NewGame };

  static constexpr int WIDTH = 320;
  static constexpr int HEIGHT = 256;
  static constexpr int DISPLAY_LINE = 50;
  static constexpr int HAND = 10;

  ContinueScene(StreetHost &host, session::GameSession &session);

  void advance(int16_t joystick);
  void compose(std::vector<uint32_t> &frame) const;
  systems::graphics::Display output() const;

  Outcome outcome() const;
  bool isShown() const;
  bool isContinueChosen() const;
  const core::BobLayer &bobs() const;
  const effects::color::AmigaPalette &palette() const;

private:
  enum class Step { Open, Choose, Chosen, Gone, Closed, Finished };
  enum class Flow { Continue, Yield };

  void open();
  Flow choose(int16_t joystick);
  void close();
  void leave();
  void redraw();

  StreetHost &m_host;
  session::GameSession &m_session;
  amal::Machine m_machine;
  core::ImageBank m_images;
  core::BobLayer m_bobs;
  core::IndexedSurface m_screen;
  core::IndexedSurface m_display;
  effects::color::AmigaPalette m_palette;

  Step m_step = Step::Open;
  Outcome m_outcome = Outcome::Choosing;
  bool m_continue = true;
  bool m_shown = false;
  int m_frame = 0;
  int m_resumeFrame = 0;
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SCENES_CONTINUESCENE_H_
