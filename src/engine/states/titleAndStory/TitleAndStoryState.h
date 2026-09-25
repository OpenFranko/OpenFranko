#ifndef ENGINE_STATES_TITLEANDSTORYSTATE_H_
#define ENGINE_STATES_TITLEANDSTORYSTATE_H_

#include "../../../systems/Bitmap.h"
#include "../../../systems/Canvas.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/FotoSequence.h"
#include "../../effects/StorySequence.h"
#include "../IEngineState.h"

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

  struct StoryImage {
    explicit StoryImage(const char *name) : resource(name) {}

    const char *resource;
    int index = -1;
    systems::IndexedBitmap bitmap;
  };

  std::optional<EngineStateEnum> runTitle();
  std::optional<EngineStateEnum> runStory();
  void drawStory(const effects::StorySequence::View &view);
  void drawStoryImage(StoryImage &image, int index, int x, int y, bool masked);

  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  systems::IndexedBitmap m_titlePicture;
  StoryImage m_frame{"03BE"};
  StoryImage m_picture{"03BF"};
  StoryImage m_text{"03C0"};
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
