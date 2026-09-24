#include "../../../../src/engine/amal/Actors.h"
#include "../../../../src/engine/amal/Machine.h"
#include <catch2/catch_all.hpp>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace openfranko::src::engine::amal;

namespace {

constexpr int RA = 0;
constexpr int RB = 1;
constexpr int RC = 2;
constexpr int RD = 3;
constexpr int RM = 12;
constexpr int RU = 20;
constexpr int RZ = 25;
constexpr int FRAME_LIMIT = 1000;

constexpr int16_t JOY_UP = 1;
constexpr int16_t JOY_DOWN = 2;
constexpr int16_t JOY_RIGHT = 8;
constexpr int16_t JOY_FIRE = 16;

struct Run {
  std::vector<int16_t> xs;
  std::vector<int16_t> ys;
  std::vector<int16_t> images;
};

Run run(Machine &machine, const Object &object, int frames) {
  Run result;
  for (int frame = 0; frame < frames; ++frame) {
    machine.tick();
    result.xs.push_back(object.x);
    result.ys.push_back(object.y);
    result.images.push_back(object.image);
  }
  return result;
}

std::vector<std::pair<int16_t, int>> holds(const std::vector<int16_t> &values) {
  std::vector<std::pair<int16_t, int>> result;
  for (int16_t value : values) {
    if (result.empty() || result.back().first != value) {
      result.emplace_back(value, 0);
    }
    ++result.back().second;
  }
  return result;
}

} // namespace

SCENARIO("Move is AMOS's 16.16 fixed point interpolation") {
  GIVEN("A bob at x 200 and M-64,0,16") {
    Registers globals{};
    Machine machine(globals);
    Object bob{200, 0, 0};
    machine.bind(1, &bob);
    machine.create(1, "M-64,0,16;");
    machine.start(1);

    WHEN("It runs for 16 frames") {
      const auto frames = run(machine, bob, 16);

      THEN("It steps 4 px from the very first frame and lands on 136") {
        int16_t previous = 200;
        for (int16_t x : frames.xs) {
          REQUIRE(previous - x == 4);
          previous = x;
        }
        REQUIRE(frames.xs.back() == 136);
      }
    }
  }

  GIVEN("A Move over no frames") {
    Registers globals{};
    Machine machine(globals);
    Object bob{10, 10, 0};
    machine.bind(1, &bob);
    machine.create(1, "M5,-3,0;LX=0;");
    machine.start(1);

    THEN("It is clamped to one frame taking the whole distance") {
      machine.tick();
      REQUIRE(bob.x == 15);
      REQUIRE(bob.y == 7);
      machine.tick();
      REQUIRE(bob.x == 0);
    }
  }

  GIVEN("Steps too large for the 16-bit step word") {
    Registers globals{};
    Machine machine(globals);
    Object flipped{100, 0, 0};
    Object overflowed{100, 0, 0};
    machine.bind(1, &flipped);
    machine.bind(2, &overflowed);
    machine.create(1, "M200,0,1;");
    machine.create(2, "M300,0,1;");
    machine.startAll();
    machine.tick();

    THEN("A quotient past 32767 flips sign and one past 65535 is dropped") {
      REQUIRE(flipped.x == 100 - 56);
      REQUIRE(overflowed.x == 100);
    }
  }
}

SCENARIO("TRZES shakes the screen in three four-frame jolts") {
  GIVEN("The shake bound to the display at y 47") {
    Registers globals{};
    Machine machine(globals);
    Object display{128, 47, 0};
    machine.bind(0, &display);
    machine.create(0, actors::screenShake());
    machine.start(0);

    WHEN("Nothing asks for a shake") {
      const auto frames = run(machine, display, 20);

      THEN("The idle loop yields on its jump budget and nothing moves") {
        REQUIRE(frames.ys == std::vector<int16_t>(20, 47));
      }
    }

    WHEN("RM is set") {
      globals[RM] = 1;
      const auto frames = run(machine, display, 13);

      THEN("The looping Next waits a frame, so each jolt is 8, 4, 0, 0") {
        const std::vector<int16_t> jolts = {55, 51, 47, 47, 55, 51, 47,
                                            47, 55, 51, 47, 47, 47};
        REQUIRE(frames.ys == jolts);
        REQUIRE(globals[RM] == 0);
      }
    }
  }
}

SCENARIO("KREW animates while it moves") {
  GIVEN("The player's blood, triggered with RZ 60") {
    Registers globals{};
    Machine machine(globals);
    Object blood{100, 100, 10};
    machine.bind(15, &blood);
    machine.create(15, actors::playerBlood());
    machine.start(15);
    globals[RZ] = 60;
    globals[RA] = 100;
    globals[RB] = 200;
    globals[RC] = 0;

    WHEN("It runs") {
      const auto frames = run(machine, blood, 30);

      THEN("The splat image 9 arrives on frame 15, with the arc") {
        REQUIRE(frames.images[14] == 8);
        REQUIRE(frames.images[15] == 9);
        REQUIRE(frames.ys[1] == 144);
        REQUIRE(frames.ys[14] == 200);
      }

      THEN("Images 2 to 8 are each held their full 2 frames") {
        const auto runs = holds(frames.images);
        REQUIRE(runs[1] == std::make_pair<int16_t, int>(2, 2));
        REQUIRE(runs[4] == std::make_pair<int16_t, int>(5, 2));
        REQUIRE(runs[7] == std::make_pair<int16_t, int>(8, 2));
      }

      THEN("It hides itself and clears RZ when done") {
        REQUIRE(frames.images[21] == 10);
        REQUIRE(globals[RZ] == 0);
      }
    }
  }
}

SCENARIO("Anim follows AmAni and AmDoAni") {
  GIVEN("A1,(5,3)(6,3) followed by a Pause loop") {
    Registers globals{};
    Machine machine(globals);
    Object bob{};
    machine.bind(1, &bob);
    machine.create(1, "A1,(5,3)(6,3);A:P;JA;");
    machine.start(1);
    const auto frames = run(machine, bob, 10);

    THEN("The first image shows on the Anim's own frame for its full delay") {
      REQUIRE(frames.images ==
              std::vector<int16_t>{5, 5, 5, 6, 6, 6, 6, 6, 6, 6});
    }
  }

  GIVEN("An endless animation on a program that has ended") {
    Registers globals{};
    Machine machine(globals);
    Object bob{};
    machine.bind(13, &bob);
    machine.create(13, actors::indicatorArrow(0));
    machine.start(13);
    const auto frames = run(machine, bob, 45);

    THEN("It keeps blinking 1 and 10 every 10 frames") {
      REQUIRE_FALSE(machine.isRunning(13));
      const auto runs = holds(frames.images);
      REQUIRE(runs.size() == 5);
      REQUIRE(runs[0] == std::make_pair<int16_t, int>(1, 10));
      REQUIRE(runs[1] == std::make_pair<int16_t, int>(10, 10));
      REQUIRE(runs[2] == std::make_pair<int16_t, int>(1, 10));
    }
  }

  GIVEN("The stage 2 arrow, spliced with Hex$ of -32768") {
    Registers globals{};
    Machine machine(globals);
    Object bob{};
    machine.bind(13, &bob);
    machine.create(13, actors::indicatorArrow(-32768));
    machine.start(13);
    machine.tick();

    THEN("It shows image 1 mirrored") {
      REQUIRE(static_cast<uint16_t>(bob.image) == 0x8001);
    }
  }
}

SCENARIO("Channels follow Amal, Amal On, Amal Freeze and Amal Off") {
  GIVEN("A channel just created with Amal") {
    Registers globals{};
    Machine machine(globals);
    Object bob{};
    machine.bind(1, &bob);
    machine.create(1, "M10,0,10;LX=99;");
    run(machine, bob, 3);

    THEN("It does not run until Amal On") {
      REQUIRE(bob.x == 0);
      REQUIRE_FALSE(machine.isRunning(1));
    }

    WHEN("It is started, frozen mid-move and started again") {
      machine.start(1);
      run(machine, bob, 4);
      machine.freeze(1);
      run(machine, bob, 5);
      const int16_t frozenAt = bob.x;
      machine.start(1);
      const auto frames = run(machine, bob, 7);

      THEN("The move resumes exactly where it stopped") {
        REQUIRE(frozenAt == 4);
        REQUIRE(frames.xs[5] == 10);
        REQUIRE(frames.xs[6] == 99);
        REQUIRE_FALSE(machine.isRunning(1));
      }
    }

    WHEN("It is created again") {
      machine.start(1);
      machine.channelRegister(1, 3) = 7;
      machine.create(1, "LX=1;");

      THEN("Its registers are fresh and it is frozen again") {
        REQUIRE(machine.channelRegister(1, 3) == 0);
        REQUIRE(machine.isFrozen(1));
      }
    }

    WHEN("It is switched off") {
      machine.destroy(1);

      THEN("Its registers can no longer be read") {
        REQUIRE_FALSE(machine.exists(1));
        REQUIRE_THROWS_AS(machine.channelRegister(1, 0), std::out_of_range);
      }
    }
  }
}

SCENARIO("Expressions follow AMAL's rules") {
  GIVEN("A program of assorted expressions") {
    Registers globals{};
    globals[RC] = static_cast<int16_t>(0x8000);
    Machine machine(globals);
    machine.create(1, "LR0=1+2*3;LR1=5=5;LR2=7/0;LR3=6<>6;LR4=$8000-RC;"
                      "LR5=-16;LR6=12&10|1;LR7=-7/2;LR8=300*300;");
    machine.start(1);
    machine.tick();

    THEN("They evaluate left to right in 16 bits, comparisons giving -1") {
      REQUIRE(machine.channelRegister(1, 0) == 9);
      REQUIRE(machine.channelRegister(1, 1) == -1);
      REQUIRE(machine.channelRegister(1, 2) == 7);
      REQUIRE(machine.channelRegister(1, 3) == 0);
      REQUIRE(machine.channelRegister(1, 4) == 0);
      REQUIRE(machine.channelRegister(1, 5) == -16);
      REQUIRE(machine.channelRegister(1, 6) == 9);
      REQUIRE(machine.channelRegister(1, 7) == -3);
      REQUIRE(machine.channelRegister(1, 8) == 24464);
    }
  }

  GIVEN("A test on the joystick") {
    Registers globals{};
    Machine machine(globals);
    machine.create(1, "A:IJ1<>16JA;LR0=1;");
    machine.start(1);

    THEN("J1 is read as the joystick, not as a jump") {
      machine.tick();
      REQUIRE(machine.channelRegister(1, 0) == 0);
      machine.setJoystick(JOY_FIRE);
      machine.tick();
      REQUIRE(machine.channelRegister(1, 0) == 1);
    }
  }

  GIVEN("Negative constants after operators") {
    Registers globals{};
    Machine machine(globals);
    machine.create(1, "LR9=-70;LR0=R9>-80;LR1=R9<-80;LR2=10--3;LR3=4*-$10;"
                      "LR4=R9-80;");
    machine.start(1);
    machine.tick();

    THEN("AniOpe reads the minus as the constant's sign, so R9>-80 compares "
         "with -80") {
      REQUIRE(machine.channelRegister(1, 0) == -1);
      REQUIRE(machine.channelRegister(1, 1) == 0);
      REQUIRE(machine.channelRegister(1, 2) == 13);
      REQUIRE(machine.channelRegister(1, 3) == -64);
      REQUIRE(machine.channelRegister(1, 4) == -150);
    }
  }
}

SCENARIO("A run-over walker slides off the left edge and ends") {
  GIVEN("Walker type 9 squashed at x 103 while RU is 5") {
    Registers globals{};
    globals[RU] = 5;
    Machine machine(globals);
    Object walker{103, 150, 9};
    machine.bind(1, &walker);
    machine.create(1, actors::pedestrian(9));
    machine.channelRegister(1, 4) = 1;
    machine.start(1);

    WHEN("It runs") {
      int frames = 0;
      while (machine.isRunning(1) && frames < FRAME_LIMIT) {
        machine.tick();
        ++frames;
      }

      THEN("IX>-80JC lets it go at the first x past -80") {
        REQUIRE_FALSE(machine.isRunning(1));
        REQUIRE(walker.x == -82);
      }
    }
  }
}

SCENARIO("The scheduler's budget and loops") {
  GIVEN("A loop with no Pause") {
    Registers globals{};
    Machine machine(globals);
    machine.create(1, "A:LR0=R0+1;JA;");
    machine.start(1);

    THEN("It gets ten jumps a frame, the tenth still taken") {
      machine.tick();
      REQUIRE(machine.channelRegister(1, 0) == 10);
      machine.tick();
      REQUIRE(machine.channelRegister(1, 0) == 20);
    }
  }

  GIVEN("A For loop whose body never pauses") {
    Registers globals{};
    Machine machine(globals);
    machine.create(1, "FR0=1T3;LR1=R1+1;NR0;LR2=1;");
    machine.start(1);

    THEN("Each iteration takes a frame and the loop ends after the third") {
      machine.tick();
      REQUIRE(machine.channelRegister(1, 1) == 1);
      machine.tick();
      REQUIRE(machine.channelRegister(1, 1) == 2);
      machine.tick();
      REQUIRE(machine.channelRegister(1, 1) == 3);
      REQUIRE(machine.channelRegister(1, 2) == 1);
    }
  }

  GIVEN("Two channels and a global register") {
    Registers globals{};
    Machine machine(globals);
    machine.create(2, "LR0=RZ;");
    machine.create(1, "LRZ=5;");
    machine.startAll();
    machine.tick();

    THEN("They run in ascending channel order, sharing RA to RZ") {
      REQUIRE(machine.channelRegister(2, 0) == 5);
      REQUIRE(globals[RZ] == 5);
    }
  }
}

SCENARIO("FRAN plays the street player") {
  GIVEN("The stage 1 player at (160,172) facing right") {
    Registers globals{};
    Machine machine(globals);
    Object player{160, 172, 17};
    machine.bind(1, &player);
    const auto programs = actors::streetPlayer(1);
    machine.create(1, programs.locomotion);
    machine.start(1);

    WHEN("Right is held") {
      machine.setJoystick(JOY_RIGHT);
      const auto frames = run(machine, player, 30);

      THEN("It decides once every 3 frames, walking 6 px a step") {
        std::vector<int> steps;
        for (std::size_t i = 1; i < frames.xs.size(); ++i) {
          if (frames.xs[i] != frames.xs[i - 1]) {
            steps.push_back(frames.xs[i] - frames.xs[i - 1]);
          }
        }
        REQUIRE(steps == std::vector<int>(9, 6));
      }
    }

    WHEN("Fire is held") {
      machine.setJoystick(JOY_FIRE);
      const auto frames = run(machine, player, 33);

      THEN("It punches: 20, 16, 31, 16 held 7 frames each") {
        const auto runs = holds(frames.images);
        REQUIRE(runs[1] == std::make_pair<int16_t, int>(20, 7));
        REQUIRE(runs[2] == std::make_pair<int16_t, int>(16, 7));
        REQUIRE(runs[3] == std::make_pair<int16_t, int>(31, 7));
        REQUIRE(runs[4] == std::make_pair<int16_t, int>(16, 7));
      }
    }

    WHEN("Up or down is held for long") {
      machine.setJoystick(JOY_UP);
      const auto upward = run(machine, player, 300);
      machine.setJoystick(JOY_DOWN);
      const auto downward = run(machine, player, 300);

      THEN("The bounds are tested before stepping: the band is 168 to 216") {
        REQUIRE(upward.ys.back() == 168);
        REQUIRE(downward.ys.back() == 216);
      }
    }
  }

  GIVEN("A punch in progress") {
    Registers globals{};
    Machine machine(globals);
    Object player{160, 172, 17};
    machine.bind(1, &player);
    machine.create(1, actors::streetPlayer(1).locomotion);
    machine.start(1);
    machine.setJoystick(JOY_FIRE);
    run(machine, player, 6);

    THEN("RD holds the attack and is cleared when it ends") {
      REQUIRE(globals[RD] == 1);
      machine.setJoystick(0);
      run(machine, player, 30);
      REQUIRE(globals[RD] == 0);
    }
  }
}

SCENARIO("The parser refuses what AMAL would not compile") {
  THEN("An undefined label, a stray Next and an unknown instruction throw") {
    REQUIRE_THROWS_AS(parse("JB;"), std::invalid_argument);
    REQUIRE_THROWS_AS(parse("NR0;"), std::invalid_argument);
    REQUIRE_THROWS_AS(parse("Q;"), std::invalid_argument);
  }

  THEN("Operands and operators must alternate, and only a constant takes a "
       "sign") {
    REQUIRE_THROWS_AS(parse("LR0=;"), std::invalid_argument);
    REQUIRE_THROWS_AS(parse("LR0=1+;"), std::invalid_argument);
    REQUIRE_THROWS_AS(parse("LR0=1>+2;"), std::invalid_argument);
    REQUIRE_THROWS_AS(parse("LR0=-R1;"), std::invalid_argument);
    REQUIRE_THROWS_AS(parse("LR0=R1R2;"), std::invalid_argument);
  }

  THEN("Lower-case letters are ignored, so words read as instructions") {
    const Program program = parse("Move 5,0,1; Pause; Jump B; B: Let R0=1");
    REQUIRE(program.code.size() == 4);
    REQUIRE(program.code[0].opcode == Opcode::Move);
    REQUIRE(program.code[2].jump == 3);
  }
}
