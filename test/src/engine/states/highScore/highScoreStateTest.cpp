#include "../../../../../src/engine/states/highScore/HighScoreState.h"

#include "../../../../../src/engine/amal/Machine.h"
#include "../../../../../src/engine/street/core/HighScoreTable.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../street/scenes/FakeStreetHost.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <optional>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::amal;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::highScore;
using namespace openfranko::src::engine::street::core;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::engine::street::scenes;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr int KEY_LIMIT = 100;

struct Board {
  explicit Board(int kills, bool ntsc = false) {
    options.ntsc = ntsc;
    session.registers[RN] = static_cast<int16_t>(kills);
    session.keyboard.permit();
    state.emplace(
        monitor, host, options, session,
        [this](const HighScoreTable &table) { saves.push_back(table); });
  }

  void type(const std::string &keys) {
    for (const char key : keys) {
      session.keyboard.press(key);
      for (int frame = 0; frame < KEY_LIMIT && !session.keyboard.isEmpty();
           ++frame) {
        state->update();
      }
    }
  }

  FakeMonitor monitor;
  FakeStreetHost host;
  GameOptions options;
  GameSession session;
  std::vector<HighScoreTable> saves;
  std::optional<HighScoreState> state;
};

} // namespace

SCENARIO("The scores are shown in the options' standard") {
  GIVEN("PAL") {
    Board board(12);
    run(*board.state, 1);

    THEN("The 320 pixel screen from line 50 shows all 256 rows") {
      REQUIRE_FALSE(board.monitor.ntsc);
      REQUIRE(board.monitor.width == 320);
      REQUIRE(board.monitor.height == 256);
    }
  }

  GIVEN("NTSC") {
    Board board(12, true);
    run(*board.state, 1);

    THEN("The monitor is switched and the screen cut to NTSC's lines") {
      REQUIRE(board.monitor.ntsc);
      REQUIRE(board.monitor.height == 236);
    }
  }
}

SCENARIO("A run without kills goes back to the menu unsaved") {
  GIVEN("RN at 0") {
    Board board(0);
    const Exit exit = runToExit(*board.state, 1000);

    THEN("The menu follows and nothing is saved") {
      REQUIRE(exit.next == EngineStateId::Menu);
      REQUIRE(board.saves.empty());
    }
  }
}

SCENARIO("Typed text is wanted only while a name is entered") {
  GIVEN("A run without kills, whose table takes no name") {
    Board board(0);
    bool asked = false;
    for (int frame = 0; frame < 1000; ++frame) {
      board.state->update();
      asked = asked || board.state->isEnteringText();
    }

    THEN("No text is asked for") { REQUIRE_FALSE(asked); }
  }

  GIVEN("12 kills in the top slot") {
    Board board(12);
    bool askedEarly = false;
    for (int frame = 0; frame < 1000 && !board.state->scene().isEntering();
         ++frame) {
      askedEarly = askedEarly || board.state->isEnteringText();
      board.state->update();
    }

    THEN("Text is asked for from the start of the entry to its end") {
      REQUIRE_FALSE(askedEarly);
      REQUIRE(board.state->isEnteringText());
      board.type("NO\r");
      REQUIRE_FALSE(board.state->isEnteringText());
    }
  }
}

SCENARIO("A name typed into the table is saved once") {
  GIVEN("12 kills in the top slot") {
    Board board(12);
    for (int frame = 0; frame < 1000 && !board.state->scene().isEntering();
         ++frame) {
      board.state->update();
    }
    REQUIRE(board.state->scene().isEntering());

    WHEN("A name is typed and committed") {
      board.type("NO\r");
      const Exit exit = runToExit(*board.state, 1000);

      THEN("The saved table holds it and the continue question follows") {
        REQUIRE(exit.next == EngineStateId::Continue);
        REQUIRE(board.saves.size() == 1);
        REQUIRE(board.saves.front().score(0) == 12);
        REQUIRE(board.saves.front().letter(0, 0) == 'N' - 'A');
        REQUIRE(board.saves.front().letter(0, 1) == 'O' - 'A');
      }
    }
  }
}
