#ifndef ENGINE_STATES_TITLEANDSTORYSTATE_H_
#define ENGINE_STATES_TITLEANDSTORYSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../GameVersion.h"
#include "../../effects/sequences/BlyskSequence.h"
#include "../../effects/sequences/FotoSequence.h"
#include "../../effects/sequences/StorySequence.h"
#include "../IEngineState.h"
#include "../shared/IntroStrip.h"
#include "../shared/MusicFadeOut.h"

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
  void drawStory(const effects::sequences::StorySequence::View &view);
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
  effects::sequences::FotoSequence m_title;
  effects::sequences::StorySequence m_story;
  std::optional<shared::IntroStrip> m_strip;
  std::optional<effects::sequences::BlyskSequence> m_pages;
  shared::MusicFadeOut m_musicFade;
  Phase m_phase = Phase::Title;
  int m_phaseFrames = 0;
  bool m_stripShown = false;
  effects::color::AmigaColor m_background = 0x000;
  effects::sequences::StorySequence::View m_lastView;
};

} // namespace titleAndStory
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_TITLEANDSTORYSTATE_H_
