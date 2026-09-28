#ifndef ENGINE_STATES_PRESENTS_INTROSTRIP_H_
#define ENGINE_STATES_PRESENTS_INTROSTRIP_H_

#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../effects/color/AmigaDisplay.h"
#include "../../effects/sequences/BlyskSequence.h"
#include "../../street/scenes/EndingCredits.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

class IntroStrip {
public:
  static constexpr int PAGES_BEFORE_KNEE = 2;

  explicit IntroStrip(systems::VideoSystem &videoSystem,
                      const std::string &directory = "assets");

  int pages() const;
  void show(const effects::sequences::BlyskSequence &sequence);
  void showBlack();

private:
  void paste(int page);
  const systems::IndexedBitmap *glyph(int image);

  systems::VideoSystem &m_videoSystem;
  std::string m_directory;
  std::vector<street::CreditPage> m_pages;
  std::map<int, std::optional<systems::IndexedBitmap>> m_glyphs;
  effects::color::VisibleRows m_rows;
  systems::Canvas m_frame;
  systems::IndexedBitmap m_strip;
  std::optional<int> m_pasted;
};

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_PRESENTS_INTROSTRIP_H_
