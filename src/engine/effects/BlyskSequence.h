#ifndef ENGINE_EFFECTS_BLYSKSEQUENCE_H_
#define ENGINE_EFFECTS_BLYSKSEQUENCE_H_

#include "AmigaPalette.h"
#include "PaletteFader.h"

#include <cstddef>
#include <optional>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

class BlyskSequence {
public:
  static constexpr std::size_t COLORS = 4;
  static constexpr int FADE_SPEED = 2;
  static constexpr int LIT_FRAMES = 60;
  static constexpr int DARK_FRAMES = 30;
  static constexpr int PAGE_FRAMES = LIT_FRAMES + DARK_FRAMES;

  BlyskSequence(int firstPage, int endPage);

  void advance(bool fireLatched);

  std::optional<int> page() const;
  const AmigaPalette &palette() const;
  bool isFinished() const;
  bool isSkipped() const;

private:
  void startPage();

  int m_page;
  int m_endPage;
  int m_time = 0;
  bool m_pasted = false;
  bool m_finished = false;
  bool m_skipped = false;
  AmigaPalette m_palette = AmigaPalette(COLORS, 0);
  PaletteFader m_fader;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_BLYSKSEQUENCE_H_
