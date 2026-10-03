#ifndef ENGINE_STATES_TITLEANDSTORY_TITLEANDSTORYSTATE_H_
#define ENGINE_STATES_TITLEANDSTORY_TITLEANDSTORYSTATE_H_

#include "../../../systems/audio/Speaker.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/Monitor.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../GameVersion.h"
#include "../../assets/Files.h"
#include "../../effects/sequences/BlyskSequence.h"
#include "../../effects/sequences/FotoSequence.h"
#include "../../effects/sequences/StorySequence.h"
#include "../EngineState.h"
#include "../shared/IntroStrip.h"
#include "../shared/MusicFadeOut.h"

#include <optional>
#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace titleAndStory {

class TitleAndStoryState : public EngineState {
public:
  TitleAndStoryState(systems::graphics::Monitor &monitor,
                     systems::audio::Speaker &speaker,
                     systems::input::ControllerSystem &controllerSystem,
                     assets::Files &files,
                     GameVersion version = GameVersion::V10);

  std::optional<EngineStateId> update() override;

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
    StoryImage(int resourceId, GameVersion version);

    std::string resource;
    int index = -1;
    systems::graphics::IndexedBitmap bitmap;
  };

  std::optional<EngineStateId> runTitle();
  std::optional<EngineStateId> runPages();
  std::optional<EngineStateId> runStory();
  std::optional<EngineStateId> leave();
  void fillScreen(effects::color::AmigaColor color);
  void drawStory(const effects::sequences::StorySequence::View &view);
  void drawStoryImage(StoryImage &image, int index, int x, int y, bool masked);

  systems::graphics::Monitor &m_monitor;
  systems::audio::Speaker &m_speaker;
  systems::input::ControllerSystem &m_controllerSystem;
  assets::Files &m_files;
  GameVersion m_version;
  systems::graphics::IndexedBitmap m_titlePicture;
  StoryImage m_frame;
  StoryImage m_picture;
  StoryImage m_text;
  systems::graphics::Canvas m_screen;
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
  std::optional<effects::sequences::StorySequence::View> m_drawnView;
  bool m_titleDrawn = false;
};

} // namespace titleAndStory
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_TITLEANDSTORY_TITLEANDSTORYSTATE_H_
