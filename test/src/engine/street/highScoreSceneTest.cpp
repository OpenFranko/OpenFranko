#include "../../../../src/engine/street/HighScoreScene.h"
#include "../../../../src/engine/street/StageFrame.h"
#include <catch2/catch_all.hpp>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street;

namespace {

constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RN = 13;
constexpr int RO = 14;

constexpr int MUSIC = 1 + LoadingMock::FILE_FRAMES;
constexpr int LOADED = MUSIC + 2 + 3 * LoadingMock::FILE_FRAMES;
constexpr int DIMMED = LOADED + 13;
constexpr int RELIT = LOADED + 16;
constexpr int ENTRY = RELIT + 100;
constexpr int CURSOR_IMAGE = 40;
constexpr uint8_t PAPER = 0;

uint8_t inkOf(int image) { return static_cast<uint8_t>(1 + image % 31); }

Picture box(int width, int height, uint8_t color) {
  return Picture{
      width, height, 0, 0,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height), color)};
}

effects::AmigaPalette picturePalette() {
  effects::AmigaPalette palette(32, 0x888);
  palette[0] = 0x444;
  palette[3] = 0xFFF;
  palette[29] = 0x760;
  palette[30] = 0xB95;
  palette[31] = 0xFC0;
  return palette;
}

HighScoreTable ladder(int lowest) {
  HighScoreTable table;
  for (int step = 0; step < HighScoreTable::ROWS; ++step) {
    table.insert(lowest + step * 10);
  }
  return table;
}

class FakeHost : public StreetHost {
public:
  std::vector<std::pair<int, int>> spriteSets;
  std::vector<int> pictures;
  std::vector<int> palettes;
  std::vector<int> music;
  std::vector<int> volumes;
  int musicStarts = 0;
  int musicStops = 0;

  std::vector<Picture> loadSpriteSet(int resource, int sampleBank) override {
    spriteSets.emplace_back(resource, sampleBank);
    std::vector<Picture> frames;
    for (int image = 1; image <= 41; ++image) {
      frames.push_back(image == CURSOR_IMAGE ? box(16, 5, inkOf(image))
                                             : box(10, 15, inkOf(image)));
    }
    return frames;
  }

  Picture loadPicture(int resource) override {
    pictures.push_back(resource);
    return box(HighScoreScene::WIDTH, HighScoreScene::HEIGHT, PAPER);
  }

  effects::AmigaPalette loadPalette(int resource) override {
    palettes.push_back(resource);
    return picturePalette();
  }

  std::vector<Picture> loadScenery(int) override { return {}; }

  LevelScript loadLevelScript(int) override { return LevelScript{}; }

  EndingCredits loadEndingCredits() override { return {}; }

  Picture loadPanelPicture(int) override { return box(304, 48, 7); }

  void loadMusic(int resource) override { music.push_back(resource); }

  void playMusic() override { ++musicStarts; }

  void stopMusic() override { ++musicStops; }

  void setMusicVolume(int volume) override { volumes.push_back(volume); }

  void playSample(int, int, int) override {}

  void playSampleAt(int, int, int, int) override {}

  void setSampleLoop(bool) override {}

  int random(int) override { return 0; }
};

struct Board {
  FakeHost host;
  GameSession session;
  effects::GameOptions options;
  std::vector<HighScoreTable> saves;
  HighScoreScene scene{
      host, session, options,
      [this](const HighScoreTable &table) { saves.push_back(table); }};

  explicit Board(int kills) {
    session.registers[RN] = static_cast<int16_t>(kills);
  }

  void run(int frames) {
    for (int frame = 0; frame < frames; ++frame) {
      scene.advance('\0');
    }
  }

  void type(const std::string &keys) {
    for (char key : keys) {
      scene.advance(key);
    }
  }

  int runUntil(const std::function<bool()> &done, int limit) {
    for (int frame = 0; frame < limit; ++frame) {
      scene.advance('\0');
      if (done()) {
        return frame + 1;
      }
    }
    return -1;
  }

  uint8_t ink(int x, int y) const { return scene.screen().pixel(x, y); }

  uint32_t pixel(int x, int y) const {
    std::vector<uint32_t> frame;
    scene.compose(frame);
    return frame[static_cast<std::size_t>(y * HighScoreScene::WIDTH + x)];
  }
};

} // namespace

SCENARIO("State 05 resets the run and reloads the menu music first") {
  GIVEN("A run that ended with 12 kills") {
    Board board(12);
    for (int i = 0; i < 26; ++i) {
      if (i != RN) {
        board.session.registers[i] = 7;
      }
    }
    board.run(1);

    THEN("Every register but RN is wiped, RO is -1, energy and lives full") {
      for (int i = 0; i < 26; ++i) {
        if (i == RN) {
          REQUIRE(board.session.registers[i] == 12);
        } else if (i == RO) {
          REQUIRE(board.session.registers[i] == -1);
        } else if (i == RF) {
          REQUIRE(board.session.registers[i] == 64);
        } else if (i == RG) {
          REQUIRE(board.session.registers[i] == 3);
        } else {
          REQUIRE(board.session.registers[i] == 0);
        }
      }
    }

    THEN("ERA has silenced the old tune and the menu tune is loading") {
      REQUIRE(board.host.musicStops == 1);
      REQUIRE(board.host.music == std::vector<int>{0x261});
      REQUIRE(board.host.musicStarts == 0);
      REQUIRE_FALSE(board.scene.isShown());
      REQUIRE(board.pixel(0, 0) == 0xFF000000u);
    }

    THEN("Music 1 starts at Mvolume 63 once the file is in") {
      board.run(MUSIC - 2);
      REQUIRE(board.host.musicStarts == 0);
      board.run(1);
      REQUIRE(board.host.musicStarts == 1);
      REQUIRE(board.host.volumes.back() == 63);
    }

    THEN("After Wait 2 the title, the picture and the letters load in turn") {
      board.run(MUSIC);
      REQUIRE(board.host.pictures.empty());
      board.run(1);
      REQUIRE(board.host.pictures == std::vector<int>{0x3BA});
      board.run(LOADED - MUSIC - 2);
      REQUIRE(board.host.pictures == std::vector<int>{0x3BA, 0x3B9});
      REQUIRE(board.host.palettes == std::vector<int>{0x3B9});
      REQUIRE(board.host.spriteSets ==
              std::vector<std::pair<int, int>>{{0x35, 5}});
    }
  }

  GIVEN("The music switched off in the menu") {
    Board board(12);
    board.options.music = false;
    board.run(MUSIC);

    THEN("Mvolume 63+63*(MUZ=0) is 0") {
      REQUIRE(board.host.musicStarts == 1);
      REQUIRE(board.host.volumes.back() == 0);
    }
  }
}

SCENARIO("A run without kills goes to the menu once the files are in") {
  GIVEN("RN at 0, as after Esc") {
    Board board(0);
    const HighScoreTable::Bytes before = board.session.highScores.bytes();

    THEN("Nothing is shown and the table is untouched") {
      board.run(LOADED - 1);
      REQUIRE(board.scene.outcome() == HighScoreScene::Outcome::Running);
      board.run(1);
      REQUIRE(board.scene.outcome() == HighScoreScene::Outcome::Menu);
      REQUIRE_FALSE(board.scene.isShown());
      REQUIRE(board.session.highScores.bytes() == before);
    }
  }
}

SCENARIO("HISHOW dims the picture four steps, relights 29-31, draws upwards") {
  GIVEN("12 kills against the seeded table") {
    Board board(12);
    board.run(LOADED);

    THEN("The score has taken the top slot") {
      REQUIRE(board.scene.slot() == 0);
      REQUIRE(board.session.highScores.score(0) == 12);
    }

    THEN("Auto View Off keeps the screen hidden until BACK's View") {
      REQUIRE_FALSE(board.scene.isShown());
      board.run(1);
      REQUIRE(board.scene.isShown());
      REQUIRE(board.scene.palette()[3] == 0xEEE);
      REQUIRE(board.pixel(0, 0) == toArgb(0x333));
    }

    THEN("Each round takes one step off every colour, four in all") {
      board.run(DIMMED - LOADED - 1);
      REQUIRE(board.scene.palette()[3] == 0xCCC);
      board.run(1);
      REQUIRE(board.scene.palette()[3] == 0xBBB);
      REQUIRE(board.scene.palette()[0] == 0x000);
      REQUIRE(board.scene.palette()[29] == 0x320);
    }

    THEN("Colours 29-31 are relit and the bottom row appears together") {
      board.run(RELIT - LOADED - 1);
      REQUIRE(board.scene.rowsShown() == 0);
      board.run(1);
      REQUIRE(board.scene.palette()[29] == 0x769);
      REQUIRE(board.scene.palette()[30] == 0xB95);
      REQUIRE(board.scene.palette()[31] == 0xFC0);
      REQUIRE(board.scene.palette()[3] == 0xBBB);
      REQUIRE(board.scene.rowsShown() == 1);
      REQUIRE(board.ink(56, 212) == inkOf('W' - 'A' + 14));
      REQUIRE(board.ink(106, 212) == PAPER);
      REQUIRE(board.ink(242, 212) == inkOf('0' - 44));
    }

    THEN("A row every 10 frames, the new one last with its score at 242") {
      board.run(RELIT - LOADED + 9);
      REQUIRE(board.scene.rowsShown() == 1);
      board.run(1);
      REQUIRE(board.scene.rowsShown() == 2);
      board.run(80);
      REQUIRE(board.scene.rowsShown() == 10);
      REQUIRE(board.ink(56, 32) == PAPER);
      REQUIRE(board.ink(232, 32) == inkOf('1' - 44));
      REQUIRE(board.ink(242, 32) == inkOf('2' - 44));
      REQUIRE_FALSE(board.scene.isEntering());
    }
  }
}

SCENARIO("The name is typed over the row the score went into") {
  GIVEN("12 kills in the top slot and HISHOW finished") {
    Board board(12);
    board.run(ENTRY - 1);
    REQUIRE_FALSE(board.scene.isEntering());
    board.run(1);

    THEN("The cursor sits under the first cell") {
      REQUIRE(board.scene.isEntering());
      REQUIRE(board.scene.bobs().isActive(1));
      REQUIRE(board.scene.bobs().x(1) == 56);
      REQUIRE(board.scene.bobs().y(1) == 47);
      REQUIRE(board.scene.bobs().image(1) == CURSOR_IMAGE);
    }

    WHEN("A letter is typed") {
      board.type("N");

      THEN("Its image A-51 is pasted at the cursor, which moves next pass") {
        REQUIRE(board.ink(56, 32) == inkOf('N' - 51));
        REQUIRE(board.scene.name().front() == 'N');
        REQUIRE(board.scene.bobs().x(1) == 56);
        board.run(1);
        REQUIRE(board.scene.bobs().x(1) == 66);
      }
    }

    WHEN("A space and a lower-case letter follow") {
      board.type("N o");

      THEN("Space only moves on and Upper$ takes the letter") {
        REQUIRE(board.ink(66, 32) == PAPER);
        REQUIRE(board.ink(76, 32) == inkOf('O' - 51));
        REQUIRE(board.scene.name().substr(0, 4) == "N O ");
      }

      AND_WHEN("Backspace is pressed twice") {
        board.type("\b");
        const bool untouched = board.ink(76, 32) == inkOf('O' - 51);
        board.type("\b");

        THEN("The first clears the empty cell under the cursor, the next "
             "the O") {
          REQUIRE(untouched);
          REQUIRE(board.ink(76, 32) == PAPER);
          REQUIRE(board.scene.name().substr(0, 4) == "N   ");
          board.run(1);
          REQUIRE(board.scene.bobs().x(1) == 66);
        }
      }
    }

    WHEN("Sixteen letters are typed") {
      board.type("ABCDEFGHIJKLMNOP");

      THEN("The cursor stops at the fifteenth cell, which takes the last") {
        REQUIRE(board.scene.name() == "ABCDEFGHIJKLMNP");
        REQUIRE(board.ink(196, 32) == inkOf('P' - 51));
      }
    }

    WHEN("Backspace is pressed on the first cell") {
      board.type("\b");
      board.run(1);

      THEN("The cursor stays") { REQUIRE(board.scene.bobs().x(1) == 56); }
    }

    WHEN("Return commits the name") {
      board.type("N O\r");

      THEN("The letters and spaces are poked and the table saved once") {
        REQUIRE(board.saves.size() == 1);
        const HighScoreTable &saved = board.saves.front();
        REQUIRE(saved.letter(0, 0) == 'N' - 'A');
        REQUIRE(saved.letter(0, 1) == 0xDF);
        REQUIRE(saved.letter(0, 2) == 'O' - 'A');
        REQUIRE(saved.letter(0, 14) == 0xDF);
        REQUIRE(saved.score(0) == 12);
        REQUIRE(board.session.highScores.bytes() == saved.bytes());
      }

      THEN("Bob Off removes the cursor and entry is over") {
        REQUIRE_FALSE(board.scene.bobs().isActive(1));
        REQUIRE_FALSE(board.scene.isEntering());
      }

      THEN("Wait 300, Fade 2 and Wait 30 lead to a black Cls 0") {
        const int frames = board.runUntil(
            [&] {
              return board.scene.outcome() != HighScoreScene::Outcome::Running;
            },
            400);
        REQUIRE(frames == 330);
        REQUIRE(board.scene.outcome() == HighScoreScene::Outcome::Continue);
        REQUIRE(board.scene.palette() == effects::AmigaPalette(32, 0x000));
        REQUIRE(board.ink(56, 32) == 0);
        REQUIRE(board.saves.size() == 1);
      }
    }
  }
}

SCENARIO("A score below the whole table is only shown") {
  GIVEN("Scores 100 down to 10 and a run of 5 kills") {
    Board board(5);
    board.session.highScores = ladder(10);
    const HighScoreTable::Bytes before = board.session.highScores.bytes();
    board.run(ENTRY);

    THEN("There is no slot, no entry and no save") {
      REQUIRE(board.scene.slot() == HighScoreTable::NO_SLOT);
      REQUIRE_FALSE(board.scene.isEntering());
      REQUIRE(board.session.highScores.bytes() == before);
    }

    THEN("Fade 2 steps from the frame after Wait 300, one step per two") {
      board.run(300);
      REQUIRE(board.scene.palette()[31] == 0xFC0);
      board.run(1);
      REQUIRE(board.scene.palette()[31] == 0xEB0);
      board.run(27);
      REQUIRE(board.scene.palette()[31] == 0x100);
      board.run(1);
      REQUIRE(board.scene.palette()[31] == 0x000);
      REQUIRE(board.scene.outcome() == HighScoreScene::Outcome::Running);
      board.run(1);
      REQUIRE(board.scene.outcome() == HighScoreScene::Outcome::Continue);
      REQUIRE(board.saves.empty());
    }
  }
}
