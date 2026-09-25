#include "TitleAndStoryState.h"

#include "../../effects/AmigaDisplay.h"

#include <array>
#include <cstddef>
#include <string>

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

constexpr effects::AmigaColor STORY_BACKGROUND_GREY = 0x444;
constexpr int STORY_SCREENS = 2;

std::string assetPath(const std::string &resource, int index) {
  const std::string file =
      index == 0 ? resource : resource + "_" + std::to_string(index);
  return "assets/" + resource + "/" + file + ".bmp";
}

std::vector<systems::IndexedBitmap> loadImages(const std::string &resource,
                                               int count) {
  std::vector<systems::IndexedBitmap> images;
  for (int index = 0; index < count; ++index) {
    images.push_back(systems::loadIndexedBitmap(assetPath(resource, index)));
  }
  return images;
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
      m_frames(loadImages("03BE", ANIMATION_FRAMES)),
      m_pictures(loadImages("03BF", PICTURES)),
      m_texts(loadImages("03C0", PICTURES)),
      m_screen(SCREEN_WIDTH, SCREEN_HEIGHT),
      m_title(screenPalette(m_titlePicture), TITLE_TIMINGS),
      m_story(PAGES, CLOSING_PICTURE) {}

std::optional<EngineStateEnum> TitleAndStoryState::update() {
  const std::optional<EngineStateEnum> next = runTitle();
  if (!next) {
    m_videoSystem.show(m_screen.pixels().data(), m_screen.width(),
                       m_screen.height());
  }
  return next;
}

std::optional<EngineStateEnum> TitleAndStoryState::runTitle() {
  if (m_phase == Phase::Title) {
    if (!m_title.isFinished()) {
      m_title.advance();
      if (m_title.isShown()) {
        m_screen.draw(m_titlePicture, m_title.palette(), 0, 0);
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
    const systems::IndexedBitmap &frame = m_frames.at(*view.frame - 1);
    m_screen.draw(frame, frame.palette, FRAME_POSITION.x, FRAME_POSITION.y);
  }
  if (view.picture) {
    const systems::IndexedBitmap &picture = m_pictures.at(*view.picture);
    m_screen.draw(picture, picture.palette, PICTURE_POSITION.x,
                  PICTURE_POSITION.y);
  }
  if (view.text) {
    const systems::IndexedBitmap &text = m_texts.at(*view.text);
    const Position &position = TEXT_POSITIONS.at(*view.text);
    m_screen.drawMasked(text, text.palette, position.x, position.y);
  }
}

} // namespace openfranko::src::engine::states::titleAndStory
