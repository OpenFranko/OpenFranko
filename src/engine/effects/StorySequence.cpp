#include "StorySequence.h"

#include <utility>

namespace openfranko::src::engine::effects {
namespace {

constexpr int NO_FRAMES = -1;

int frameCount(const StorySequence::Page &page) {
  return page.lastFrame - page.firstFrame + 1;
}

} // namespace

StorySequence::StorySequence(std::vector<Page> animatedPages,
                             int closingPicture)
    : m_pages(std::move(animatedPages)) {
  m_pages.push_back({0, NO_FRAMES, closingPicture});
}

void StorySequence::advance(bool skipLatched, bool joystickTouched) {
  if (!m_finished) {
    step(skipLatched, joystickTouched);
  }
}

const StorySequence::View &StorySequence::view() const { return m_view; }

bool StorySequence::isFinished() const { return m_finished; }

void StorySequence::step(bool skipLatched, bool joystickTouched) {
  const Page &page = m_pages[m_page];
  const int time = m_time++;
  const int animationEnd = FRAMES_PER_ANIMATION_FRAME * frameCount(page);

  if (time <= animationEnd && time % FRAMES_PER_ANIMATION_FRAME == 0) {
    if (skipLatched) {
      finish();
      return;
    }
    if (time < animationEnd) {
      m_view.frame = page.firstFrame + time / FRAMES_PER_ANIMATION_FRAME;
    }
  }

  const int pictureStart = animationEnd + PICTURE_FRAMES;
  const int textStart = pictureStart + TEXT_FRAMES;
  const int readingStart = textStart + PAUSE_FRAMES;
  if (time == pictureStart) {
    m_view.picture = page.picture;
  }
  if (time == textStart) {
    m_view.text = page.picture;
  }
  if (time >= readingStart &&
      (joystickTouched || time == readingStart + READING_FRAMES)) {
    nextPage(skipLatched, joystickTouched);
  }
}

void StorySequence::nextPage(bool skipLatched, bool joystickTouched) {
  m_view = {};
  if (++m_page == m_pages.size()) {
    finish();
    return;
  }
  m_time = 0;
  step(skipLatched, joystickTouched);
}

void StorySequence::finish() {
  m_view = {};
  m_finished = true;
}

} // namespace openfranko::src::engine::effects
