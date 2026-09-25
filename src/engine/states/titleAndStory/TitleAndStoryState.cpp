#include "TitleAndStoryState.h"

#include "../../effects/AmigaDisplay.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace openfranko::src::engine::states::titleAndStory {
namespace {

constexpr auto TITLE = "title";
constexpr auto TITLE_PATH = "assets/03BA.bmp";

constexpr int SCREEN_ID = 0;
constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 256;
constexpr std::size_t SCREEN_COLORS = 32;

constexpr int LOADING_FRAMES = 300;
constexpr effects::FotoSequence::Timings TITLE_TIMINGS{5, LOADING_FRAMES, 3, 45,
                                                       false};

const std::vector<effects::StorySequence::Page> PAGES = {
    {1, 8, 0}, {8, 11, 1}, {11, 14, 2}, {14, 20, 3}, {20, 28, 4}, {28, 69, 5}};
constexpr int CLOSING_PICTURE = 6;
constexpr int ANIMATION_FRAMES = 69;
constexpr int PICTURES = 7;

struct Position {
  int x;
  int y;
};

constexpr Position FRAME_POSITION{0, 72};
constexpr Position PICTURE_POSITION{96, 84};
constexpr std::array<Position, PICTURES> TEXT_POSITIONS = {
    {{0, 0}, {8, 8}, {16, 7}, {0, 7}, {24, 14}, {16, 0}, {8, 12}}};

constexpr uint8_t STORY_BACKGROUND_GREY = 0x44;
constexpr int STORY_SCREENS = 2;

std::string assetPath(const std::string &resource, int index) {
  const std::string file =
      index == 0 ? resource : resource + "_" + std::to_string(index);
  return "assets/" + resource + "/" + file + ".bmp";
}

std::string frameName(int frame) {
  return "storyFrame" + std::to_string(frame);
}

std::string pictureName(int picture) {
  return "storyPicture" + std::to_string(picture);
}

std::string textName(int text) { return "storyText" + std::to_string(text); }

effects::AmigaPalette openScreen(systems::VideoSystem &videoSystem) {
  videoSystem.createScreen(SCREEN_ID, SCREEN_WIDTH, SCREEN_HEIGHT);
  videoSystem.switchScreen(SCREEN_ID);
  videoSystem.loadIndexedImage(TITLE, TITLE_PATH);

  auto palette = videoSystem.getImagePalette(TITLE);
  palette.resize(SCREEN_COLORS);
  return palette;
}

bool isJoystickTouched(
    const systems::ControllerSystem::ControllerStates &states) {
  return states.up || states.down || states.left || states.right ||
         states.button;
}

void drawStory(systems::VideoSystem &videoSystem,
               const effects::StorySequence::View &view) {
  videoSystem.fillScreen(STORY_BACKGROUND_GREY, STORY_BACKGROUND_GREY,
                         STORY_BACKGROUND_GREY);
  if (view.frame) {
    videoSystem.drawImage(frameName(*view.frame), FRAME_POSITION.x,
                          FRAME_POSITION.y);
  }
  if (view.picture) {
    videoSystem.drawImage(pictureName(*view.picture), PICTURE_POSITION.x,
                          PICTURE_POSITION.y);
  }
  if (view.text) {
    const Position &position = TEXT_POSITIONS.at(*view.text);
    videoSystem.drawImage(textName(*view.text), position.x, position.y);
  }
}

} // namespace

TitleAndStoryState::TitleAndStoryState(
    systems::VideoSystem &videoSystem,
    systems::ControllerSystem &controllerSystem)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_title(openScreen(videoSystem), TITLE_TIMINGS),
      m_story(PAGES, CLOSING_PICTURE) {
  m_videoSystem.setImagePalette(TITLE, m_title.palette());
  for (int frame = 1; frame <= ANIMATION_FRAMES; ++frame) {
    m_videoSystem.loadImage(frameName(frame), assetPath("03BE", frame - 1));
  }
  for (int picture = 0; picture < PICTURES; ++picture) {
    m_videoSystem.loadImage(pictureName(picture), assetPath("03BF", picture));
    m_videoSystem.loadMaskedImage(textName(picture),
                                  assetPath("03C0", picture));
  }
}

TitleAndStoryState::~TitleAndStoryState() {
  m_videoSystem.clearImage(TITLE);
  for (int frame = 1; frame <= ANIMATION_FRAMES; ++frame) {
    m_videoSystem.clearImage(frameName(frame));
  }
  for (int picture = 0; picture < PICTURES; ++picture) {
    m_videoSystem.clearImage(pictureName(picture));
    m_videoSystem.clearImage(textName(picture));
  }
  m_videoSystem.fillScreen(0, 0, 0);
}

std::optional<EngineStateEnum> TitleAndStoryState::update() {
  if (m_phase == Phase::Title) {
    if (!m_title.isFinished()) {
      if (m_title.advance()) {
        m_videoSystem.setImagePalette(TITLE, m_title.palette());
      }
      if (m_title.isShown()) {
        m_videoSystem.drawImage(TITLE, 0, 0);
      } else {
        m_videoSystem.fillScreen(0, 0, 0);
      }
      return std::nullopt;
    }
    if (m_controllerSystem.isFireLatched()) {
      return EngineStateEnum::ProtectionCheck;
    }
    m_phase = Phase::StoryOpening;
    m_phaseFrames = 0;
  }
  return runStory();
}

std::optional<EngineStateEnum> TitleAndStoryState::runStory() {
  if (m_phase == Phase::StoryOpening) {
    if (m_phaseFrames < STORY_SCREENS * effects::SCREEN_OPEN_VBLS) {
      if (m_phaseFrames == 0) {
        m_videoSystem.fillScreen(0, 0, 0);
      } else {
        m_videoSystem.fillScreen(STORY_BACKGROUND_GREY, STORY_BACKGROUND_GREY,
                                 STORY_BACKGROUND_GREY);
      }
      ++m_phaseFrames;
      return std::nullopt;
    }
    m_phase = Phase::Story;
  }
  if (m_phase == Phase::Story) {
    const effects::StorySequence::View shown = m_story.view();
    m_story.advance(m_controllerSystem.isFireLatched(),
                    isJoystickTouched(m_controllerSystem.states));
    if (!m_story.isFinished()) {
      drawStory(m_videoSystem, m_story.view());
      return std::nullopt;
    }
    m_lastView = shown;
    m_phase = Phase::StoryClosing;
    m_phaseFrames = 0;
  }
  if (m_phaseFrames == STORY_SCREENS * effects::SCREEN_CLOSE_VBLS) {
    return EngineStateEnum::ProtectionCheck;
  }
  if (m_phaseFrames < effects::SCREEN_CLOSE_SHOWN_VBLS) {
    drawStory(m_videoSystem, m_lastView);
  } else {
    m_videoSystem.fillScreen(STORY_BACKGROUND_GREY, STORY_BACKGROUND_GREY,
                             STORY_BACKGROUND_GREY);
  }
  ++m_phaseFrames;
  return std::nullopt;
}

} // namespace openfranko::src::engine::states::titleAndStory
