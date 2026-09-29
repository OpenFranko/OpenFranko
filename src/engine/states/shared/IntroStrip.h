#ifndef ENGINE_STATES_SHARED_INTROSTRIP_H_
#define ENGINE_STATES_SHARED_INTROSTRIP_H_

#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../AmigaDisplay.h"
#include "../../effects/sequences/BlyskSequence.h"
#include "../../street/core/EndingCredits.h"

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

  explicit IntroStrip(systems::graphics::VideoSystem &videoSystem,
                      const std::string &directory = "assets");

  int pages() const;
  void show(const effects::sequences::BlyskSequence &sequence);
  void showBlack();

private:
  void paste(int page);
  void pasteGlyph(int x, int y, int image);
  const systems::graphics::IndexedBitmap *glyph(int image);

  systems::graphics::VideoSystem &m_videoSystem;
  std::string m_directory;
  std::vector<street::core::CreditPage> m_pages;
  std::map<int, std::optional<systems::graphics::IndexedBitmap>> m_glyphs;
  VisibleRows m_rows;
  systems::graphics::Canvas m_frame;
  systems::graphics::IndexedBitmap m_strip;
  std::optional<int> m_pasted;
};

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SHARED_INTROSTRIP_H_
