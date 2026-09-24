#ifndef ENGINE_EFFECTS_STORYSEQUENCE_H_
#define ENGINE_EFFECTS_STORYSEQUENCE_H_

#include <cstddef>
#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

class StorySequence {
public:
  struct Page {
    int firstFrame;
    int lastFrame;
    int picture;
  };

  struct View {
    std::optional<int> frame;
    std::optional<int> picture;
    std::optional<int> text;
  };

  static constexpr int FRAMES_PER_ANIMATION_FRAME = 8;
  static constexpr int PICTURE_FRAMES = 3;
  static constexpr int TEXT_FRAMES = 15;
  static constexpr int PAUSE_FRAMES = 10;
  static constexpr int READING_FRAMES = 2000;

  StorySequence(std::vector<Page> animatedPages, int closingPicture);

  void advance(bool skipLatched, bool joystickTouched);

  const View &view() const;
  bool isFinished() const;

private:
  void step(bool skipLatched, bool joystickTouched);
  void nextPage(bool skipLatched, bool joystickTouched);
  void finish();

  std::vector<Page> m_pages;
  std::size_t m_page = 0;
  int m_time = 0;
  View m_view;
  bool m_finished = false;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_STORYSEQUENCE_H_
