#ifndef ENGINE_STREET_CONTINUESCENE_H_
#define ENGINE_STREET_CONTINUESCENE_H_

#include "../amal/Machine.h"
#include "../effects/AmigaPalette.h"
#include "Bobs.h"
#include "GameSession.h"
#include "IndexedSurface.h"
#include "StreetStage.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

class ContinueScene {
public:
  enum class Outcome { Choosing, Continue, NewGame };

  static constexpr int WIDTH = 320;
  static constexpr int HEIGHT = 256;
  static constexpr int HAND = 10;

  ContinueScene(StreetHost &host, GameSession &session);

  void advance(int16_t joystick);
  void compose(std::vector<uint32_t> &frame) const;

  Outcome outcome() const;
  bool isShown() const;
  bool isContinueChosen() const;
  const BobLayer &bobs() const;
  const effects::AmigaPalette &palette() const;

private:
  enum class Step { Open, Choose, Chosen, Finished };
  enum class Flow { Continue, Yield };

  void open();
  Flow choose(int16_t joystick);
  void close();
  void redraw();

  GameSession &m_session;
  amal::Machine m_machine;
  ImageBank m_images;
  BobLayer m_bobs;
  IndexedSurface m_screen;
  IndexedSurface m_display;
  effects::AmigaPalette m_palette;

  Step m_step = Step::Open;
  Outcome m_outcome = Outcome::Choosing;
  bool m_continue = true;
  bool m_shown = false;
  int m_frame = 0;
  int m_resumeFrame = 0;
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CONTINUESCENE_H_
