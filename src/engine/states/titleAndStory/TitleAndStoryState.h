#ifndef ENGINE_STATES_TITLEANDSTORYSTATE_H_
#define ENGINE_STATES_TITLEANDSTORYSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/Bitmap.h"
#include "../../../systems/Canvas.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../GameVersion.h"
#include "../../effects/BlyskSequence.h"
#include "../../effects/FotoSequence.h"
#include "../../effects/StorySequence.h"
#include "../IEngineState.h"
#include "../presents/IntroStrip.h"
#include "../presents/MusicFadeOut.h"

#include <optional>
#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace titleAndStory {

class TitleAndStoryState : public IEngineState {
public:
  TitleAndStoryState(systems::VideoSystem &videoSystem,
                     systems::AudioSystem &audioSystem,
                     systems::ControllerSystem &controllerSystem,
                     GameVersion version = GameVersion::V10);

  std::optional<EngineStateEnum> update() override;

private:
  enum class Phase {
    Title,
    Pages,
    StripClosing,
    StoryOpening,
    Story,
    StoryClosing,
    MusicFade
  };

  struct StoryImage {
    StoryImage(int resource, GameVersion version);

    std::string resource;
    int index = -1;
    systems::IndexedBitmap bitmap;
  };

  std::optional<EngineStateEnum> runTitle();
  std::optional<EngineStateEnum> runPages();
  std::optional<EngineStateEnum> runStory();
  std::optional<EngineStateEnum> leave();
  void drawStory(const effects::StorySequence::View &view);
  void drawStoryImage(StoryImage &image, int index, int x, int y, bool masked);

  systems::VideoSystem &m_videoSystem;
  systems::AudioSystem &m_audioSystem;
  systems::ControllerSystem &m_controllerSystem;
  GameVersion m_version;
  systems::IndexedBitmap m_titlePicture;
  StoryImage m_frame;
  StoryImage m_picture;
  StoryImage m_text;
  systems::Canvas m_screen;
  effects::FotoSequence m_title;
  effects::StorySequence m_story;
  std::optional<presents::IntroStrip> m_strip;
  std::optional<effects::BlyskSequence> m_pages;
  presents::MusicFadeOut m_musicFade;
  Phase m_phase = Phase::Title;
  int m_phaseFrames = 0;
  bool m_stripShown = false;
  effects::AmigaColor m_background = 0x000;
  effects::StorySequence::View m_lastView;
};

} // namespace titleAndStory
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_TITLEANDSTORYSTATE_H_
