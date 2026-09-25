#include "TitleAndStoryState.h"

#include "../../effects/AmigaDisplay.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace openfranko::src::engine::states::titleAndStory {
namespace {

constexpr auto TITLE_PATH = "assets/03BA.bmp";

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 256;
constexpr std::size_t SCREEN_COLORS = 32;
constexpr effects::AmigaColor BLACK = 0x000;

constexpr int LOADING_FRAMES = 300;
constexpr effects::FotoSequence::Timings TITLE_TIMINGS{5, LOADING_FRAMES, 3, 45,
                                                       false};

const std::vector<effects::StorySequence::Page> PAGES = {
    {1, 8, 0}, {8, 11, 1}, {11, 14, 2}, {14, 20, 3}, {20, 28, 4}, {28, 69, 5}};
constexpr int CLOSING_PICTURE = 6;
constexpr int PICTURES = 7;

struct Position {
  int x;
  int y;
};

constexpr Position FRAME_POSITION{0, 72};
constexpr Position PICTURE_POSITION{96, 84};
constexpr std::array<Position, PICTURES> TEXT_POSITIONS = {
    {{0, 0}, {8, 8}, {16, 7}, {0, 7}, {24, 14}, {16, 0}, {8, 12}}};

constexpr effects::AmigaColor STORY_BACKGROUND_GREY = 0x444;
constexpr int STORY_SCREENS = 2;

std::string assetPath(const std::string &resource, int index) {
  const std::string file =
      index == 0 ? resource : resource + "_" + std::to_string(index);
  return "assets/" + resource + "/" + file + ".bmp";
}

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

TitleAndStoryState::TitleAndStoryState(
    systems::VideoSystem &videoSystem,
    systems::ControllerSystem &controllerSystem)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_titlePicture(systems::loadIndexedBitmap(TITLE_PATH)),
      m_screen(SCREEN_WIDTH, SCREEN_HEIGHT),
      m_title(screenPalette(m_titlePicture), TITLE_TIMINGS),
      m_story(PAGES, CLOSING_PICTURE) {}

std::optional<EngineStateEnum> TitleAndStoryState::update() {
  const std::optional<EngineStateEnum> next = runTitle();
  if (!next) {
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
    if (m_controllerSystem.isFireLatched()) {
      return EngineStateEnum::ProtectionCheck;
    }
    m_phase = Phase::StoryOpening;
    m_phaseFrames = 0;
    m_titlePicture = systems::IndexedBitmap{};
  }
  return runStory();
}

std::optional<EngineStateEnum> TitleAndStoryState::runStory() {
  if (m_phase == Phase::StoryOpening) {
    if (m_phaseFrames < STORY_SCREENS * effects::SCREEN_OPEN_VBLS) {
      m_screen.fill(m_phaseFrames == 0 ? BLACK : STORY_BACKGROUND_GREY);
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
      drawStory(m_story.view());
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
    drawStory(m_lastView);
  } else {
    m_screen.fill(STORY_BACKGROUND_GREY);
  }
  ++m_phaseFrames;
  return std::nullopt;
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
    const Position &position = TEXT_POSITIONS.at(*view.text);
    drawStoryImage(m_text, *view.text, position.x, position.y, true);
  }
}

void TitleAndStoryState::drawStoryImage(StoryImage &image, int index, int x,
                                        int y, bool masked) {
  if (image.index != index) {
    image.bitmap = systems::loadIndexedBitmap(assetPath(image.resource, index));
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
