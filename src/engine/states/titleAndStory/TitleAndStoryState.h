#ifndef ENGINE_STATES_TITLEANDSTORYSTATE_H_
#define ENGINE_STATES_TITLEANDSTORYSTATE_H_

#include "../../../systems/Bitmap.h"
#include "../../../systems/Canvas.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/FotoSequence.h"
#include "../../effects/StorySequence.h"
#include "../IEngineState.h"

#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace titleAndStory {

class TitleAndStoryState : public IEngineState {
public:
  TitleAndStoryState(systems::VideoSystem &videoSystem,
                     systems::ControllerSystem &controllerSystem);

  std::optional<EngineStateEnum> update() override;

private:
  enum class Phase { Title, StoryOpening, Story, StoryClosing };

  std::optional<EngineStateEnum> runTitle();
  std::optional<EngineStateEnum> runStory();
  void drawStory(const effects::StorySequence::View &view);

  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  systems::IndexedBitmap m_titlePicture;
  std::vector<systems::IndexedBitmap> m_frames;
  std::vector<systems::IndexedBitmap> m_pictures;
  std::vector<systems::IndexedBitmap> m_texts;
  systems::Canvas m_screen;
  effects::FotoSequence m_title;
  effects::StorySequence m_story;
  Phase m_phase = Phase::Title;
  int m_phaseFrames = 0;
  effects::StorySequence::View m_lastView;
};

} // namespace titleAndStory
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_TITLEANDSTORYSTATE_H_
