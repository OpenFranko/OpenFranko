#include "../../../../src/engine/effects/StorySequence.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::src::engine::effects;

namespace {

const std::vector<StorySequence::Page> PAGES = {
    {1, 8, 0}, {8, 11, 1}, {11, 14, 2}, {14, 20, 3}, {20, 28, 4}, {28, 69, 5}};
constexpr int CLOSING_PICTURE = 6;

constexpr int FRAME = StorySequence::FRAMES_PER_ANIMATION_FRAME;
constexpr int PAGE_TAIL =
    StorySequence::PICTURE_FRAMES + StorySequence::TEXT_FRAMES +
    StorySequence::PAUSE_FRAMES + StorySequence::READING_FRAMES;
constexpr int FIRST_PAGE_PICTURE = 8 * FRAME + StorySequence::PICTURE_FRAMES;
constexpr int FIRST_PAGE_TEXT = FIRST_PAGE_PICTURE + StorySequence::TEXT_FRAMES;
constexpr int FIRST_PAGE_READING_START =
    FIRST_PAGE_TEXT + StorySequence::PAUSE_FRAMES;
constexpr int FIRST_PAGE_END =
    FIRST_PAGE_READING_START + StorySequence::READING_FRAMES;

std::vector<StorySequence::View> run(StorySequence &story, int frames,
                                     bool skipLatched = false,
                                     bool joystickTouched = false) {
  std::vector<StorySequence::View> views;
  for (int frame = 0; frame < frames; ++frame) {
    story.advance(skipLatched, joystickTouched);
    views.push_back(story.view());
  }
  return views;
}

bool isBlank(const StorySequence::View &view) {
  return !view.frame && !view.picture && !view.text;
}

struct ReadPage {
  std::vector<int> frames;
  int picture = -1;
};

ReadPage readPage(StorySequence &story) {
  ReadPage page;
  for (int guard = 0; !story.view().text && guard < 1000; ++guard) {
    story.advance(false, false);
    const StorySequence::View &view = story.view();
    if (view.frame &&
        (page.frames.empty() || page.frames.back() != *view.frame)) {
      page.frames.push_back(*view.frame);
    }
    if (view.picture) {
      page.picture = *view.picture;
    }
  }
  run(story, StorySequence::PAUSE_FRAMES - 1);
  story.advance(false, true);
  return page;
}

} // namespace

SCENARIO("StorySequence plays the story pages as ANI, TEX and KLIKER do") {
  GIVEN("The six animated pages and the closing picture") {
    StorySequence story(PAGES, CLOSING_PICTURE);

    WHEN("The first page plays without any input") {
      const auto views = run(story, FIRST_PAGE_END + FRAME + 1);

      THEN("Each frame is unpacked at once, then held for ANI's Timer>7") {
        REQUIRE(views[0].frame == 1);
        REQUIRE(views[7].frame == 1);
        REQUIRE(views[8].frame == 2);
        REQUIRE(views[55].frame == 7);
        REQUIRE(views[56].frame == 8);
        REQUIRE(views[64].frame == 8);
      }

      THEN("The picture follows the last frame, and the text the picture") {
        REQUIRE_FALSE(views[FIRST_PAGE_PICTURE - 1].picture.has_value());
        REQUIRE(views[FIRST_PAGE_PICTURE].picture == 0);
        REQUIRE(views[FIRST_PAGE_PICTURE].frame == 8);
        REQUIRE_FALSE(views[FIRST_PAGE_TEXT - 1].text.has_value());
        REQUIRE(views[FIRST_PAGE_TEXT].text == 0);
      }

      THEN("KLIKER[20] keeps the page up for 2000 frames after its Wait 10, "
           "then Cls 0 and the next ANI's first unpack come together") {
        REQUIRE(views[FIRST_PAGE_END - 1].text == 0);
        REQUIRE(views[FIRST_PAGE_END].frame == 8);
        REQUIRE_FALSE(views[FIRST_PAGE_END].picture.has_value());
        REQUIRE_FALSE(views[FIRST_PAGE_END].text.has_value());
        REQUIRE(views[FIRST_PAGE_END + FRAME - 1].frame == 8);
        REQUIRE(views[FIRST_PAGE_END + FRAME].frame == 9);
      }
    }

    WHEN("The joystick is touched while the page is read") {
      run(story, FIRST_PAGE_READING_START);
      story.advance(false, true);

      THEN("The screen clears and the next page starts at once") {
        REQUIRE(story.view().frame == 8);
        REQUIRE_FALSE(story.view().picture.has_value());
        REQUIRE_FALSE(story.view().text.has_value());
        const auto views = run(story, FRAME);
        REQUIRE(views[FRAME - 2].frame == 8);
        REQUIRE(views[FRAME - 1].frame == 9);
      }
    }

    WHEN("The joystick is touched before the text has been up for Wait 10") {
      const auto views = run(story, FIRST_PAGE_READING_START, false, true);

      THEN("It is ignored") {
        REQUIRE(views[8 * FRAME].frame == 8);
        REQUIRE(views.back().text == 0);
        REQUIRE_FALSE(story.isFinished());
      }
    }

    WHEN("Each page is read as soon as it can be") {
      std::vector<ReadPage> pages;
      for (int guard = 0; !story.isFinished() && guard < 10; ++guard) {
        pages.push_back(readPage(story));
      }

      THEN("Pictures 0 to 6 follow in order and the story ends after 6") {
        REQUIRE(story.isFinished());
        REQUIRE(pages.size() == 7);
        for (int picture = 0; picture < 7; ++picture) {
          REQUIRE(pages[picture].picture == picture);
        }
      }

      THEN("Each page plays its ANI frames, the boundary frames twice") {
        REQUIRE(pages[0].frames == std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8});
        REQUIRE(pages[1].frames == std::vector<int>{8, 9, 10, 11});
        REQUIRE(pages[4].frames.front() == 20);
        REQUIRE(pages[4].frames.back() == 28);
        REQUIRE(pages[5].frames.size() == 42);
        REQUIRE(pages[5].frames.back() == 69);
      }

      THEN("The closing page has no animation") {
        REQUIRE(pages[6].frames.empty());
      }
    }

    WHEN("Nobody touches the joystick") {
      const int storyFrames =
          StorySequence::FRAMES_PER_ANIMATION_FRAME * (8 + 4 + 4 + 7 + 9 + 42) +
          7 * PAGE_TAIL;
      run(story, storyFrames);

      THEN("The story ends when the closing page times out") {
        REQUIRE(story.view().text == CLOSING_PICTURE);
        REQUIRE_FALSE(story.view().frame.has_value());
        REQUIRE_FALSE(story.isFinished());
        story.advance(false, false);
        REQUIRE(story.isFinished());
        REQUIRE(isBlank(story.view()));
      }
    }
  }
}

SCENARIO("StorySequence stops when fire has been latched") {
  GIVEN("The story") {
    StorySequence story(PAGES, CLOSING_PICTURE);

    WHEN("Fire was latched before the story started") {
      story.advance(true, false);

      THEN("Nothing is shown") {
        REQUIRE(story.isFinished());
        REQUIRE(isBlank(story.view()));
      }
    }

    WHEN("Fire is latched while a frame is held") {
      run(story, 2 * FRAME + 1);
      const auto views = run(story, FRAME, true);

      THEN("The story ends at ANI's next check, before the next unpack") {
        REQUIRE(views[FRAME - 2].frame == 3);
        REQUIRE(isBlank(views[FRAME - 1]));
        REQUIRE(story.isFinished());
      }
    }

    WHEN("Fire is latched while the last frame of a page is held") {
      run(story, 7 * FRAME + 1);
      const auto views = run(story, FRAME, true);

      THEN("ANI's If Amreg(25)=0 skips the picture and the story ends") {
        REQUIRE(views[FRAME - 2].frame == 8);
        REQUIRE(isBlank(views[FRAME - 1]));
        REQUIRE(story.isFinished());
      }
    }

    WHEN("Fire is pressed while the page is read") {
      run(story, FIRST_PAGE_READING_START + 50);
      story.advance(true, true);

      THEN("The story ends at once") {
        REQUIRE(story.isFinished());
        REQUIRE(isBlank(story.view()));
      }
    }

    WHEN("Fire is latched while the text is built, then let go") {
      run(story, FIRST_PAGE_PICTURE);
      run(story, 5, true, true);
      const auto views =
          run(story, FIRST_PAGE_END - FIRST_PAGE_PICTURE - 5, true, false);

      THEN("KLIKER still waits out the page") {
        REQUIRE(views.back().text == 0);
        REQUIRE_FALSE(story.isFinished());
      }

      AND_WHEN("KLIKER times out") {
        story.advance(true, false);

        THEN("The story ends instead of turning the page") {
          REQUIRE(story.isFinished());
        }
      }
    }
  }
}
