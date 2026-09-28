#ifndef ENGINE_STATES_PRESENTS_INTROSTRIP_H_
#define ENGINE_STATES_PRESENTS_INTROSTRIP_H_

#include "../../../systems/Bitmap.h"
#include "../../../systems/Canvas.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/AmigaDisplay.h"
#include "../../effects/BlyskSequence.h"
#include "../../street/EndingCredits.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace presents {

class IntroStrip {
public:
  static constexpr int PAGES_BEFORE_KNEE = 2;

  explicit IntroStrip(systems::VideoSystem &videoSystem,
                      const std::string &directory = "assets");

  int pages() const;
  void show(const effects::BlyskSequence &sequence);
  void showBlack();

private:
  void paste(int page);
  const systems::IndexedBitmap *glyph(int image);

  systems::VideoSystem &m_videoSystem;
  std::string m_directory;
  std::vector<street::CreditPage> m_pages;
  std::map<int, std::optional<systems::IndexedBitmap>> m_glyphs;
  effects::VisibleRows m_rows;
  systems::Canvas m_frame;
  systems::IndexedBitmap m_strip;
  std::optional<int> m_pasted;
};

} // namespace presents
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_PRESENTS_INTROSTRIP_H_
