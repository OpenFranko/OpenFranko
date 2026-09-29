#include "../../src/engine/Engine.h"
#include "../../src/engine/GameOptions.h"
#include "../../src/engine/street/scenes/BossStage.h"
#include "../../src/engine/street/ui/StageFrame.h"
#include "../../src/engine/street/ui/StatusPanel.h"
#include "../../src/systems/graphics/Bitmap.h"

#include <cstdint>
#include <string>
#include <utility>

using namespace openfranko::src;
using namespace openfranko::src::engine;

namespace {

constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RN = 13;
constexpr int RO = 14;
constexpr int16_t LAST_STAGE = 3;
constexpr auto PANEL_DIRECTORY = "assets/0384/";

street::core::Picture panelPicture(const std::string &file) {
  systems::graphics::IndexedBitmap bitmap =
      systems::graphics::loadIndexedBitmap(PANEL_DIRECTORY + file);
  return street::core::Picture{bitmap.width, bitmap.height, bitmap.hotspotX,
                               bitmap.hotspotY, std::move(bitmap.pixels)};
}

street::session::BossExit lastBossExit(const amal::Registers &registers) {
  const street::ui::StageLayout layout = street::ui::stageLayout(GameOptions{});
  street::ui::StatusPanel panel(panelPicture("0384.bmp"),
                                panelPicture("0384_1.bmp"));
  panel.score({registers[RF], registers[RO], registers[RN], registers[RG]});
  const street::core::IndexedSurface screen(
      street::scenes::BossStage::SCREEN_WIDTH,
      street::scenes::BossStage::SCREEN_HEIGHT);
  return street::session::BossExit{street::core::DoubleBuffer(screen),
                                   street::ui::levelPalette(false),
                                   street::ui::playDisplayY(layout),
                                   0,
                                   panel.surface(),
                                   street::ui::panelDisplayY(layout),
                                   layout.laced};
}

} // namespace

int main() {
  street::session::GameSession session;
  session.registers[RO] = LAST_STAGE;
  session.bossExit.emplace(lastBossExit(session.registers));
  Engine engine(states::EngineStateEnum::Ending, std::move(session));
  engine.run();
  return 0;
}
