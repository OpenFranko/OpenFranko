#include "PresentsState.h"

#include <algorithm>

namespace openfranko::src::engine::states::presents {

PresentsState::PresentsState(systems::VideoSystem &videoSystem,
                             systems::AudioSystem &audioSystem,
                             systems::ControllerSystem &controllerSystem)
    : m_audioSystem(audioSystem), m_controllerSystem(controllerSystem),
      m_strip(videoSystem),
      m_sequence(0, std::min(IntroStrip::PAGES_BEFORE_KNEE, m_strip.pages())) {
  m_controllerSystem.clearFireLatch();
}

std::optional<EngineStateEnum> PresentsState::update() {
  if (m_musicFade) {
    m_strip.showBlack();
    if (m_musicFade->advance(m_audioSystem)) {
      return EngineStateEnum::HighScore;
    }
    return std::nullopt;
  }
  if (m_sequence.isFinished()) {
    return EngineStateEnum::KneeAnimation;
  }
  m_sequence.advance(m_controllerSystem.isFireLatched());
  if (m_sequence.isSkipped()) {
    m_musicFade.emplace();
    return update();
  }
  if (m_frame++ == 0) {
    m_strip.showBlack();
  } else {
    m_strip.show(m_sequence);
  }
  return std::nullopt;
}

} // namespace openfranko::src::engine::states::presents
