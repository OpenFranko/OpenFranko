#include "TitleAndStoryState.h"

#include "../../assets/Assets.h"
#include "../../effects/AmigaDisplay.h"

#include <array>
#include <cstddef>
#include <vector>

namespace openfranko::src::engine::states::titleAndStory {
namespace {

constexpr int TITLE = 0x3BA;
constexpr int STORY_FRAMES = 0x3BE;
constexpr int STORY_PICTURES = 0x3BF;
constexpr int STORY_TEXTS = 0x3C0;

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 256;
constexpr std::size_t SCREEN_COLORS = 32;
constexpr effects::AmigaColor BLACK = 0x000;

constexpr int LOADING_FRAMES = 300;
constexpr effects::FotoSequence::Timings TITLE_TIMINGS{5, LOADING_FRAMES, 3, 45,
                                                       false};
constexpr int VERSION12_TITLE_WAIT = 100;
constexpr effects::FotoSequence::Timings VERSION12_TITLE_TIMINGS{
    5, VERSION12_TITLE_WAIT, 3, 45, false};

const std::vector<effects::StorySequence::Page> PAGES = {
    {1, 8, 0}, {8, 11, 1}, {11, 14, 2}, {14, 20, 3}, {20, 28, 4}, {28, 69, 5}};
constexpr int CLOSING_PICTURE = 6;
constexpr int PICTURES = 7;

constexpr int VERSION12_FRAME_UNPACK = 10;
constexpr int VERSION12_FRAME_WAIT = 2;
constexpr int VERSION12_FRAMES_PER_ANIMATION_FRAME =
    VERSION12_FRAME_UNPACK + VERSION12_FRAME_WAIT;

struct Position {
  int x;
  int y;
};

constexpr Position FRAME_POSITION{0, 72};
constexpr Position PICTURE_POSITION{96, 84};
constexpr std::array<Position, PICTURES> TEXT_POSITIONS = {
    {{0, 0}, {8, 8}, {16, 7}, {0, 7}, {24, 14}, {16, 0}, {8, 12}}};
constexpr Position VERSION12_TEXT_POSITION{0, 0};

constexpr effects::AmigaColor STORY_BACKGROUND_GREY = 0x444;
constexpr int STORY_SCREENS = 2;

effects::AmigaPalette screenPalette(const systems::IndexedBitmap &picture) {
  effects::AmigaPalette palette = picture.palette;
  palette.resize(SCREEN_COLORS);
  return palette;
}

bool isJoystickTouched(
    const systems::ControllerSystem::ControllerStates &states) {
  return states.up || states.down || states.left || states.right ||
         states.button;
}

} // namespace

TitleAndStoryState::StoryImage::StoryImage(int resourceId, GameVersion version)
    : resource(assets::resourceName(resourceId, version)) {}

TitleAndStoryState::TitleAndStoryState(
    systems::VideoSystem &videoSystem, systems::AudioSystem &audioSystem,
    systems::ControllerSystem &controllerSystem, GameVersion version)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_controllerSystem(controllerSystem), m_version(version),
      m_titlePicture(systems::loadIndexedBitmap(
          assets::picturePath(assets::resourceName(TITLE, version)))),
      m_frame(STORY_FRAMES, version), m_picture(STORY_PICTURES, version),
      m_text(STORY_TEXTS, version), m_screen(SCREEN_WIDTH, SCREEN_HEIGHT),
      m_title(screenPalette(m_titlePicture), version == GameVersion::V12
                                                 ? VERSION12_TITLE_TIMINGS
                                                 : TITLE_TIMINGS),
      m_story(PAGES, CLOSING_PICTURE,
              version == GameVersion::V12
                  ? VERSION12_FRAMES_PER_ANIMATION_FRAME
                  : effects::StorySequence::FRAMES_PER_ANIMATION_FRAME) {
  if (m_version == GameVersion::V12) {
    m_strip.emplace(videoSystem);
  }
}

std::optional<EngineStateEnum> TitleAndStoryState::update() {
  m_stripShown = false;
  const std::optional<EngineStateEnum> next = runTitle();
  if (!next && !m_stripShown) {
    m_videoSystem.show(m_screen.output());
  }
  return next;
}

std::optional<EngineStateEnum> TitleAndStoryState::runTitle() {
  if (m_phase == Phase::Title) {
    if (!m_title.isFinished()) {
      m_title.advance();
      if (m_title.isShown()) {
        m_screen.setPalette(m_title.palette());
        m_screen.draw(m_titlePicture, 0, 0);
      } else {
        m_screen.fill(BLACK);
      }
      return std::nullopt;
    }
    m_titlePicture = systems::IndexedBitmap{};
    if (m_controllerSystem.isFireLatched()) {
      return leave();
    }
    m_phaseFrames = 0;
    if (m_version == GameVersion::V12) {
      m_phase = Phase::Pages;
      m_pages.emplace(presents::IntroStrip::PAGES_BEFORE_KNEE,
                      m_strip->pages());
    } else {
      m_phase = Phase::StoryOpening;
    }
  }
  if (m_phase == Phase::Pages || m_phase == Phase::StripClosing) {
    return runPages();
  }
  return runStory();
}

std::optional<EngineStateEnum> TitleAndStoryState::runPages() {
  if (m_phase == Phase::Pages) {
    m_pages->advance(m_controllerSystem.isFireLatched());
    if (m_pages->isSkipped()) {
      return leave();
    }
    if (!m_pages->isFinished()) {
      m_strip->show(*m_pages);
      m_stripShown = true;
      return std::nullopt;
    }
    m_phase = Phase::StripClosing;
    m_phaseFrames = 0;
  }
  if (m_phaseFrames < effects::SCREEN_CLOSE_VBLS) {
    m_strip->showBlack();
    m_stripShown = true;
    ++m_phaseFrames;
    return std::nullopt;
  }
  m_phase = Phase::StoryOpening;
  m_phaseFrames = 0;
  return runStory();
}

std::optional<EngineStateEnum> TitleAndStoryState::runStory() {
  if (m_phase == Phase::MusicFade) {
    m_screen.fill(m_background);
    if (m_musicFade.advance(m_audioSystem)) {
      return EngineStateEnum::HighScore;
    }
    return std::nullopt;
  }
  if (m_phase == Phase::StoryOpening) {
    m_background = STORY_BACKGROUND_GREY;
    if (m_phaseFrames < STORY_SCREENS * effects::SCREEN_OPEN_VBLS) {
      m_screen.fill(m_phaseFrames == 0 ? BLACK : STORY_BACKGROUND_GREY);
      ++m_phaseFrames;
      return std::nullopt;
    }
    m_phase = Phase::Story;
  }
  if (m_phase == Phase::Story) {
    const effects::StorySequence::View shown = m_story.view();
    if (m_version == GameVersion::V12) {
      m_story.advance(false, m_controllerSystem.states.button);
    } else {
      m_story.advance(m_controllerSystem.isFireLatched(),
                      isJoystickTouched(m_controllerSystem.states));
    }
    if (!m_story.isFinished()) {
      drawStory(m_story.view());
      return std::nullopt;
    }
    m_lastView = shown;
    m_phase = Phase::StoryClosing;
    m_phaseFrames = 0;
  }
  if (m_phaseFrames == STORY_SCREENS * effects::SCREEN_CLOSE_VBLS) {
    return leave();
  }
  if (m_phaseFrames < effects::SCREEN_CLOSE_SHOWN_VBLS) {
    drawStory(m_lastView);
  } else {
    m_screen.fill(STORY_BACKGROUND_GREY);
  }
  ++m_phaseFrames;
  return std::nullopt;
}

std::optional<EngineStateEnum> TitleAndStoryState::leave() {
  if (m_version != GameVersion::V12) {
    return EngineStateEnum::ProtectionCheck;
  }
  m_phase = Phase::MusicFade;
  return runStory();
}

void TitleAndStoryState::drawStory(const effects::StorySequence::View &view) {
  m_screen.fill(STORY_BACKGROUND_GREY);
  if (view.frame) {
    drawStoryImage(m_frame, *view.frame - 1, FRAME_POSITION.x, FRAME_POSITION.y,
                   false);
  }
  if (view.picture) {
    drawStoryImage(m_picture, *view.picture, PICTURE_POSITION.x,
                   PICTURE_POSITION.y, false);
  }
  if (view.text) {
    const Position &position = m_version == GameVersion::V12
                                   ? VERSION12_TEXT_POSITION
                                   : TEXT_POSITIONS.at(*view.text);
    drawStoryImage(m_text, *view.text, position.x, position.y, true);
  }
}

void TitleAndStoryState::drawStoryImage(StoryImage &image, int index, int x,
                                        int y, bool masked) {
  if (image.index != index) {
    image.bitmap =
        systems::loadIndexedBitmap(assets::partPath(image.resource, index));
    image.index = index;
  }
  m_screen.setPalette(image.bitmap.palette);
  if (masked) {
    m_screen.drawMasked(image.bitmap, x, y);
  } else {
    m_screen.draw(image.bitmap, x, y);
  }
}

} // namespace openfranko::src::engine::states::titleAndStory
