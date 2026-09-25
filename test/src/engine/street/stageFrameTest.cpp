#include "../../../../src/engine/street/StageFrame.h"
#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street;

namespace {

constexpr int PLAY_WIDTH = 320;
constexpr int PLAY_ROWS = 222;
constexpr int PLAY_COLORS = 15;
constexpr int PANEL_COLORS = 7;
constexpr uint32_t BLANK = 0xFF000000u;
constexpr uint32_t BORDER = 0xFF555555u;

const effects::AmigaPalette GREYS = {0x000, 0x111, 0x222, 0x333, 0x444, 0x555,
                                     0x666, 0x777, 0x888, 0x999, 0xAAA, 0xBBB,
                                     0xCCC, 0xDDD, 0xEEE, 0xFFF};

Picture rowCoded(int width, int height, int colors) {
  Picture picture{
      width, height, 0, 0,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height))};
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      picture.pixels[static_cast<std::size_t>(y * width + x)] =
          static_cast<uint8_t>(1 + y % colors);
    }
  }
  return picture;
}

uint32_t playRow(int row) {
  return toArgb(GREYS[static_cast<std::size_t>(1 + row % PLAY_COLORS)]);
}

uint32_t panelRow(int row) {
  return toArgb(
      panelPalette()[static_cast<std::size_t>(1 + row % PANEL_COLORS)]);
}

struct Stage {
  StageLayout layout;
  IndexedSurface play{PLAY_WIDTH, PLAY_ROWS};
  StatusPanel panel{
      rowCoded(StatusPanel::WIDTH, StatusPanel::HEIGHT, PANEL_COLORS),
      Picture{}};
  amal::Object display;

  explicit Stage(StageLayout stageLayout)
      : layout(stageLayout),
        display{DISPLAY_X, static_cast<int16_t>(playDisplayY(stageLayout)), 0} {
    play.unpack(rowCoded(PLAY_WIDTH, PLAY_ROWS, PLAY_COLORS), 0, 0);
    panel.showLoading();
  }

  std::vector<uint32_t> compose() const {
    std::vector<uint32_t> frame;
    composeFrame(frame, &play, GREYS, display, 0, &panel, panelDisplayY(layout),
                 panelPalette(), layout);
    return frame;
  }
};

uint32_t at(const std::vector<uint32_t> &frame, int row) {
  return frame[static_cast<std::size_t>(row * FRAME_WIDTH + 10)];
}

} // namespace

SCENARIO("The stage layout follows state 09's Screen Display lines") {
  THEN("PAL puts the play screen at 47 and the panel at 270") {
    const StageLayout pal{false, false};
    REQUIRE(playDisplayY(pal) == 47);
    REQUIRE(panelDisplayY(pal) == 270);
    REQUIRE(frameTop(pal) == 47);
    REQUIRE(frameRows(pal) == 255);
  }

  THEN("NTSC moves both up 40 lines, and the frame with them") {
    const StageLayout ntsc{true, false};
    REQUIRE(playDisplayY(ntsc) == 7);
    REQUIRE(panelDisplayY(ntsc) == 230);
    REQUIRE(frameTop(ntsc) == 7);
    REQUIRE(frameRows(ntsc) == 255);
  }

  THEN("320x512 laces the play screen 60 lines lower, the panel 51 higher, "
       "and doubles the rows") {
    const StageLayout tall{false, true};
    REQUIRE(playDisplayY(tall) == 107);
    REQUIRE(panelDisplayY(tall) == 219);
    REQUIRE(frameTop(tall) == 47);
    REQUIRE(rowsPerLine(tall) == 2);
    REQUIRE(frameRows(tall) == 510);
    const StageLayout tallNtsc{true, true};
    REQUIRE(playDisplayY(tallNtsc) == 67);
    REQUIRE(panelDisplayY(tallNtsc) == 179);
  }

  THEN("The menu's options pick the layout") {
    effects::GameOptions options;
    options.ntsc = true;
    options.tallScreen = true;
    const StageLayout layout = stageLayout(options);
    REQUIRE(layout.ntsc);
    REQUIRE(layout.laced);
  }
}

SCENARIO("composeFrame shows the screens where the display puts them") {
  GIVEN("PAL") {
    const std::vector<uint32_t> frame =
        Stage(StageLayout{false, false}).compose();

    THEN("222 play rows, one border row, then the 32 panel rows") {
      REQUIRE(frame.size() == 304u * 255u);
      REQUIRE(at(frame, 0) == playRow(0));
      REQUIRE(at(frame, 221) == playRow(221));
      REQUIRE(at(frame, 222) == BORDER);
      REQUIRE(at(frame, 223) == panelRow(0));
      REQUIRE(at(frame, 254) == panelRow(31));
    }
  }

  GIVEN("NTSC") {
    const std::vector<uint32_t> frame =
        Stage(StageLayout{true, false}).compose();

    THEN("The lines above 26 cannot be shown, so play rows 0-18 are lost") {
      REQUIRE(at(frame, 0) == BLANK);
      REQUIRE(at(frame, 18) == BLANK);
      REQUIRE(at(frame, 19) == playRow(19));
      REQUIRE(at(frame, 221) == playRow(221));
    }

    THEN("The panel ends on line 261, the last of the NTSC frame") {
      REQUIRE(at(frame, 222) == BORDER);
      REQUIRE(at(frame, 223) == panelRow(0));
      REQUIRE(at(frame, 254) == panelRow(31));
    }
  }

  GIVEN("320x512") {
    const std::vector<uint32_t> frame =
        Stage(StageLayout{false, true}).compose();

    THEN("Each line takes two rows and the laced play screen shows both "
         "fields in 111 lines") {
      REQUIRE(frame.size() == 304u * 510u);
      REQUIRE(at(frame, 119) == BORDER);
      REQUIRE(at(frame, 120) == playRow(0));
      REQUIRE(at(frame, 121) == playRow(1));
      REQUIRE(at(frame, 341) == playRow(221));
      REQUIRE(at(frame, 342) == BORDER);
    }

    THEN("The panel keeps single-height lines from line 219") {
      REQUIRE(at(frame, 344) == panelRow(0));
      REQUIRE(at(frame, 345) == panelRow(0));
      REQUIRE(at(frame, 407) == panelRow(31));
      REQUIRE(at(frame, 408) == BORDER);
    }
  }

  GIVEN("320x512 with the screen shake 8 lines down") {
    Stage stage(StageLayout{false, true});
    stage.display.y = static_cast<int16_t>(stage.display.y + 8);
    const std::vector<uint32_t> frame = stage.compose();

    THEN("The laced screen moves 16 rows") {
      REQUIRE(at(frame, 135) == BORDER);
      REQUIRE(at(frame, 136) == playRow(0));
    }
  }
}

SCENARIO("StageDisplay shows copper changes a VBL after the list is built") {
  GIVEN("A PAL display with the play screen up") {
    StageDisplay display;
    const StageCopper pal{true, {DISPLAY_X, 47, 0}, false};
    const StageCopper ntsc{true, {DISPLAY_X, 7, 0}, true};
    display.reset(pal);

    WHEN("SYS pokes BEAMCON0 and the next VBL comes") {
      display.vbl(true);

      THEN("The beam is NTSC, but both screens keep their PAL lines") {
        REQUIRE(display.window(false).ntsc);
        REQUIRE(display.live().screenDisplay.y == 47);
        REQUIRE(display.panelY(false) == 270);
      }

      AND_WHEN("A test point builds the list with SYS's Screen Display") {
        display.rebuild(ntsc);

        THEN("Nothing moves before the next VBL") {
          REQUIRE(display.live().screenDisplay.y == 47);
          REQUIRE(display.panelY(false) == 270);
        }

        AND_WHEN("The VBL comes") {
          display.vbl(true);

          THEN("Both screens are on their NTSC lines") {
            REQUIRE(display.live().screenDisplay.y == 7);
            REQUIRE(display.panelY(false) == 230);
          }
        }
      }
    }

    WHEN("A close drops the screen") {
      display.hide();

      THEN("It goes at once and stays gone after the VBL") {
        REQUIRE_FALSE(display.live().screenShown);
        display.vbl(false);
        REQUIRE_FALSE(display.live().screenShown);
      }
    }
  }
}
