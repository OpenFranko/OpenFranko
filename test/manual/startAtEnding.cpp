#include "../../src/engine/Engine.h"
#include "../../src/engine/effects/GameOptions.h"
#include "../../src/engine/street/BossStage.h"
#include "../../src/engine/street/StageFrame.h"
#include "../../src/engine/street/StatusPanel.h"
#include "../../src/systems/Bitmap.h"

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

street::Picture panelPicture(const std::string &file) {
  systems::IndexedBitmap bitmap =
      systems::loadIndexedBitmap(PANEL_DIRECTORY + file);
  return street::Picture{bitmap.width, bitmap.height, bitmap.hotspotX,
                         bitmap.hotspotY, std::move(bitmap.pixels)};
}

street::BossExit lastBossExit(const amal::Registers &registers) {
  const street::StageLayout layout =
      street::stageLayout(effects::GameOptions{});
  street::StatusPanel panel(panelPicture("0384.bmp"),
                            panelPicture("0384_1.bmp"));
  panel.score({registers[RF], registers[RO], registers[RN], registers[RG]});
  const street::IndexedSurface screen(street::BossStage::SCREEN_WIDTH,
                                      street::BossStage::SCREEN_HEIGHT);
  return street::BossExit{street::DoubleBuffer(screen),
                          street::levelPalette(false),
                          street::playDisplayY(layout),
                          0,
                          panel.surface(),
                          street::panelDisplayY(layout),
                          layout.laced};
}

} // namespace

int main() {
  street::GameSession session;
  session.registers[RO] = LAST_STAGE;
  session.bossExit.emplace(lastBossExit(session.registers));
  Engine engine(states::EngineStateEnum::Ending, std::move(session));
  engine.run();
  return 0;
}
