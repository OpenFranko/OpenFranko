#include "../../../../../src/engine/street/actors/Actors.h"
#include "../../../../../src/engine/amal/Machine.h"
#include "../../../../../src/engine/amal/Program.h"
#include "../../amal/Run.h"

#include <catch2/catch_all.hpp>

#include <string>
#include <utility>
#include <vector>

using namespace openfranko::src::engine::amal;
using namespace openfranko::src::engine::street;
using namespace openfranko::test::src::engine::amal;

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

bool contains(const std::string &program, const std::string &part) {
  return program.find(part) != std::string::npos;
}

} // namespace

SCENARIO("Hex$ and AMOS booleans splice numbers into the programs") {
  GIVEN("The helpers the programs are built with") {
    THEN("Hex$ writes upper-case hex after a dollar, negatives in 32 bits") {
      REQUIRE(actors::hex(0) == "$0");
      REQUIRE(actors::hex(15) == "$F");
      REQUIRE(actors::hex(0x76) == "$76");
      REQUIRE(actors::hex(-32768) == "$FFFF8000");
    }

    THEN("A true comparison is -1") {
      REQUIRE(actors::amosBool(true) == -1);
      REQUIRE(actors::amosBool(false) == 0);
    }
  }
}

SCENARIO("WROG builds each enemy for its sprite slot and type") {
  GIVEN("The three sprite slots") {
    const auto first = actors::enemy(0, 0);
    const auto second = actors::enemy(25, 0);
    const auto third = actors::enemy(50, 0);

    THEN("The taunt sample adds 3 or 6 because AMOS booleans are -1") {
      REQUIRE(contains(first.walk, "H:LRW=$E;P;JA;"));
      REQUIRE(contains(second.walk, "H:LRW=$11;P;JA;"));
      REQUIRE(contains(third.walk, "H:LRW=$14;P;JA;"));
    }

    THEN("Death asks for the corpse of the slot's own image 68") {
      REQUIRE(contains(first.damage, "U:LRE=$D;LA=$44+R1;"));
      REQUIRE(contains(first.damage, "LR9=1000+$44;"));
      REQUIRE(contains(second.damage, "U:LRE=$10;LA=$5D+R1;"));
      REQUIRE(contains(second.damage, "LR9=1000+$5D;"));
      REQUIRE(contains(third.damage, "U:LRE=$13;LA=$76+R1;"));
      REQUIRE(contains(third.damage, "LR9=1000+$76;"));
    }

    THEN("Walking cycles the slot's images 44 to 47") {
      REQUIRE(contains(second.walk, "LA=R0/2+$45+R2;JA;"));
      REQUIRE(contains(third.walk, "W:A2,($5E+R2,13)($5F+R2,13)($60+R2,13)("
                                   "$61+R2,13);"));
    }
  }

  GIVEN("The three enemy types") {
    const auto bald = actors::enemy(0, 0);
    const auto frog = actors::enemy(0, 1);
    const auto kid = actors::enemy(0, 2);

    THEN("Only the kid ducks under a long jump kick") {
      REQUIRE(contains(kid.walk, "IRD=4JI;"));
      REQUIRE(contains(kid.walk, "I:LR1=1;LA=$42+R2;M0,0,60;JT;"));
      REQUIRE_FALSE(contains(bald.walk, "IRD=4JI;"));
      REQUIRE_FALSE(contains(frog.walk, "IRD=4JI;"));
    }

    THEN("The vomit height depends on the type, and the kid has none") {
      REQUIRE(contains(bald.damage, "LRU=$37;LRT=$8000-R1;"));
      REQUIRE(contains(frog.damage, "LRU=$1F;LRT=$8000-R1;"));
      REQUIRE(contains(kid.damage, "Y:LRE=11;LA=$43+R1;LRM=1;JQ;"));
    }
  }

  GIVEN("Every sprite slot with every enemy type") {
    THEN("Every slot and type parses") {
      for (int base : {0, 25, 50}) {
        for (int type : {0, 1, 2}) {
          const auto programs = actors::enemy(base, type);
          REQUIRE_NOTHROW(parse(programs.walk));
          REQUIRE_NOTHROW(parse(programs.damage));
        }
      }
    }
  }
}

SCENARIO("FRAN clamps the player to the stage's arena") {
  GIVEN("FRAN for each stage") {
    THEN("Stages 1 and 3 keep X in 32..272, stage 2 in 48..288") {
      REQUIRE(actors::streetPlayer(1).clamp ==
              "A:IX<32JB;IX>272JC;JA;B:LX=32;JA;C:LX=272;JA;");
      REQUIRE(actors::streetPlayer(3).clamp == actors::streetPlayer(1).clamp);
      REQUIRE(actors::streetPlayer(2).clamp ==
              "A:IX<48JB;IX>288JC;JA;B:LX=48;JA;C:LX=288;JA;");
    }

    THEN("All three of its programs parse") {
      const auto programs = actors::streetPlayer(1);
      REQUIRE_NOTHROW(parse(programs.locomotion));
      REQUIRE_NOTHROW(parse(programs.damage));
      REQUIRE_NOTHROW(parse(programs.clamp));
    }
  }
}

SCENARIO("The small actors") {
  GIVEN("MARTWY and the indicator arrow") {
    THEN("MARTWY only pauses, and the arrow blinks the indicator") {
      REQUIRE(actors::idle() == "A:P;JA;");
      REQUIRE(actors::indicatorArrow(0) == "A0,(1+$0,10)(10,10);");
      REQUIRE_NOTHROW(parse(actors::playerBlood()));
      REQUIRE_NOTHROW(parse(actors::enemyBlood()));
      REQUIRE_NOTHROW(parse(actors::screenShake()));
    }
  }
}

SCENARIO("BFRAN is the player for boss arenas") {
  GIVEN("Stages 1 and 2") {
    const auto first = actors::bossPlayer(1);
    const auto second = actors::bossPlayer(2);

    THEN("The walking bounds move 16 px on the mirrored stage") {
      REQUIRE(contains(first.locomotion, "B:IX>$120JA;"));
      REQUIRE(contains(first.locomotion, "C:IX<$20JA;"));
      REQUIRE(contains(second.locomotion, "B:IX>$130JA;"));
      REQUIRE(contains(second.locomotion, "C:IX<$30JA;"));
    }

    THEN("The top of the arena is left to the host in R2") {
      REQUIRE(contains(first.locomotion, "E:IY<R2JM;"));
    }

    THEN("The clamp's moving bound R0 is on the side the boss comes from") {
      REQUIRE(first.clamp == "A:IX<32JB;IX>R0JC;JA;B:LX=32;JA;C:LX=R0;JA;");
      REQUIRE(second.clamp == "A:IX<R0JB;IX>288JC;JA;B:LX=R0;JA;C:LX=288;JA;");
    }

    THEN("The boss's five attacks cost 2, 3, 8, 6 and 10 energy") {
      REQUIRE(contains(first.damage, "LRF=RF-2;"));
      REQUIRE(contains(first.damage, "LRF=RF-3;"));
      REQUIRE(contains(first.damage, "LRF=RF-8;"));
      REQUIRE(contains(first.damage, "LRF=RF-6;"));
      REQUIRE(contains(first.damage, "LRF=RF-10;"));
    }
  }
}

SCENARIO("BOSS enters differently on each stage") {
  GIVEN("BOSS for each stage") {
    THEN("Stage 1 waits at X 470 and steps 8 px left per ratchet") {
      const auto boss = actors::boss(1);
      REQUIRE(boss.walk.rfind("LX=470;LY=108;LA=78;X:P;IRX=2JY;IRX=0JX;LX=X-8;",
                              0) == 0);
      REQUIRE(contains(boss.walk, "I:LA=$38+R2;M0,0,60;"));
    }

    THEN("Stage 2 comes from the left and ducks with image 61") {
      const auto boss = actors::boss(2);
      REQUIRE(boss.walk.rfind("LX=-144;LY=124;", 0) == 0);
      REQUIRE(contains(boss.walk, "LX=X+8;"));
      REQUIRE(contains(boss.walk, "I:LA=$3D+R2;M0,0,60;"));
    }

    THEN("The damage program is the same everywhere and ends on RI") {
      const auto boss = actors::boss(1);
      REQUIRE(boss.damage == actors::boss(3).damage);
      REQUIRE(contains(boss.damage, "B:LR7=R7-4;"));
      REQUIRE(contains(boss.damage, "E:LR7=R7-20;"));
      REQUIRE(boss.damage.size() >= 2);
      REQUIRE(boss.damage.compare(boss.damage.size() - 2, 2, "X:") == 0);
    }

    THEN("Every stage's programs parse") {
      for (int stage = 1; stage <= 3; ++stage) {
        const auto player = actors::bossPlayer(stage);
        const auto boss = actors::boss(stage);
        const auto talk = actors::dialogue(stage);
        REQUIRE_NOTHROW(parse(player.locomotion));
        REQUIRE_NOTHROW(parse(player.damage));
        REQUIRE_NOTHROW(parse(player.clamp));
        REQUIRE_NOTHROW(parse(boss.walk));
        REQUIRE_NOTHROW(parse(boss.damage));
        REQUIRE_NOTHROW(parse(talk.player));
        REQUIRE_NOTHROW(parse(talk.boss));
      }
    }
  }
}

SCENARIO("The spectator, the speech bubbles and the finishing moves") {
  GIVEN("The boss stage's other actors") {
    THEN("Only stages 1 and 3 have a spectator") {
      REQUIRE(actors::spectator(1).rfind("LX=176;LY=108;LA=10;", 0) == 0);
      REQUIRE(actors::spectator(2).empty());
      REQUIRE_NOTHROW(parse(actors::spectator(3)));
    }

    THEN("On stage 1 the boss speaks first, then they alternate to RT 99") {
      const auto talk = actors::dialogue(1);
      REQUIRE(talk.boss.rfind("A:P;IRT=0JA;LA=91;", 0) == 0);
      REQUIRE(talk.player.rfind("A:P;IRT=2JB;JA;B:LY=RB;LX=RA;LA=92;", 0) == 0);
      REQUIRE(contains(talk.player, "LRT=99;LA=10;"));
    }

    THEN("KONBOSS's programs parse") {
      REQUIRE_NOTHROW(parse(actors::walkToBoss()));
      REQUIRE_NOTHROW(parse(actors::finishingPose()));
      REQUIRE_NOTHROW(parse(actors::finishingBlood()));
      REQUIRE_NOTHROW(parse(actors::finishingPoseBack()));
      REQUIRE(actors::walkOff() == "A0,(11+RC,5)(12+RC,5)(13+RC,5)(14+RC,5)(15+"
                                   "RC,5)(16+RC,5);MRT,0,RU;");
    }
  }
}

SCENARIO("KONBOSS on stage 2 throws the boss overhead") {
  GIVEN("KONBOSS's stage 2 programs") {
    THEN("The boss's and the player's scripts are the source's, and parse") {
      REQUIRE(
          actors::bossThrown() ==
          "M0,0,100;LA=78+RR;M0,0,50;LA=79+RR;M0,0,20;LA=80+RR;M0,0,30;LA=77+"
          "RR;");
      REQUIRE(actors::victoryLift() ==
              "LA=38+RR;M0,0,50;LA=39+RR;M0,0,50;LA=86+RR+RT;M0,0,50;LA=87+RR+"
              "RT;M0,0,50;LA=38+RR;");
      REQUIRE_NOTHROW(parse(actors::bossThrown()));
      REQUIRE_NOTHROW(parse(actors::victoryLift()));
    }
  }
}

SCENARIO("KONBOSS on stage 3 walks the boss to the railing") {
  GIVEN("KONBOSS's stage 3 programs") {
    THEN("The boss rests until R0 is set, then walks RU,RS in RT frames") {
      REQUIRE(actors::bossRests() ==
              "LA=75+RR;LR0=0;A:P;IR0=0JA;A0,(43+RR,5)(44+RR,5)(45+RR,5)(46+"
              "RR,5);MRU,RS,RT;");
      REQUIRE_NOTHROW(parse(actors::bossRests()));
    }

    THEN("His bubble hides on fire alone") {
      REQUIRE(actors::bubbleUntilFire() == "A:P;IJ1<>16JA;LA=10;");
      REQUIRE_NOTHROW(parse(actors::bubbleUntilFire()));
    }
  }
}

SCENARIO("The bonus drive builds each pedestrian from its first image") {
  GIVEN("The bonus drive's actors") {
    THEN("A type starting at image 9 squashes through images $C and $D") {
      REQUIRE(actors::pedestrian(9) ==
              "LR3=A+3;LR2=A;FR0=0T50;LX=X-RT;LA=A+1;IA<R3JA;LA=R2;A:P;LY=Y+4;"
              "FR1=1T10;IR4=1JB;NR1;NR0;JD;B:A1,($C,10)($D,10);C:P;LX=X-RU;"
              "M0,0,7;IX>-80JC;D:");
      REQUIRE_NOTHROW(parse(actors::pedestrian(24)));
    }

    THEN("The car drives off with one long Move") {
      REQUIRE(actors::carDriveOff() == "A0,(1,10)(2,10);M800,0,400;");
      REQUIRE_NOTHROW(parse(actors::carDriveOff()));
    }
  }
}

SCENARIO("RACZKA's pointing hand waits on R1 and waggles four times") {
  GIVEN("RACZKA's program") {
    THEN("The program is the source's, and it parses") {
      REQUIRE(actors::pointingHand() ==
              "A:P;IR1=0JA;FR0=0T3;M4,0,2;M-4,0,2;NR0;LR1=0;JA;");
      REQUIRE_NOTHROW(parse(actors::pointingHand()));
    }
  }
}

SCENARIO("CONGRA's actors are the source's programs") {
  GIVEN("CONGRA's actors") {
    THEN("Franko walks away through images 3 to 24, a pixel lower each") {
      REQUIRE(actors::walkAway() ==
              "FR0=3T24;LA=R0;LY=Y+1;M0,0,20;P;NR0;LA=26;");
      REQUIRE_NOTHROW(parse(actors::walkAway()));
    }

    THEN("The break-dance is B$ and both C$ strings spliced as state 18 does") {
      REQUIRE(
          actors::breakDance() ==
          "A6,($8004,10)($8005,10)($8006,10)($8007,10)($8008,10)($8009,10);M-"
          "320,"
          "0,320;A1,($800A,1);M0,0,80;A1,($8011,1);M0,0,20;A6,(4,10)(5,10)(6,"
          "10)("
          "7,10)(8,10)(9,10);M320,0,320;A1,(10,1);A12,($8012,10)($8013,10)($"
          "8014,"
          "10)($8015,10)($8016,10)($8017,10);M-340,0,640;A1,(10,1);A6,(4,10)(5,"
          "10)(6,10)(7,10)(8,10)(9,10);M400,0,400;A1,(10,1);A12,($800B,10)($"
          "800C,"
          "10)($800D,10)($800E,10)($800F,10)($8010,10);M-200,0,400;A1,(10,1);"
          "A6,("
          "4,10)(5,10)(6,10)(7,10)(8,10)(9,10);M140,0,140;A1,(10,1);A12,($8012,"
          "10)($8013,10)($8014,10)($8015,10)($8016,10)($8017,10);M-330,0,660;"
          "A1,("
          "10,1);M0,0,20;LA=17;M0,0,380;A1,(17,50)(11,50);A "
          "4,(11,1)(12,1)(13,1)(14,1)(15,1)(16,1);M32,0,32;A4,(11,1)(12,1)(13,"
          "1)("
          "14,1)(15,1)(16,1);M-16,0,16;A4,(11,1)(12,1)(13,1)(14,1)(15,1)(16,1);"
          "M16,0,16;A4,(11,1)(12,1)(13,1)(14,1)(15,1)(16,1);M-16,0,16;A4,(11,1)"
          "("
          "12,1)(13,1)(14,1)(15,1)(16,1);M16,0,16;A1,(4,10)(5,10)(6,10)(7,10)("
          "8,"
          "10)(9,10);M64,0,64;A1,(17,1);");
      REQUIRE_NOTHROW(parse(actors::breakDance()));
    }

    THEN("The portraits wait 800, 1600 and 2400 frames before walking in") {
      REQUIRE(actors::portraitEntrance(1) ==
              "M0,0,800;M-310,0,580;M0,0,2080;M320,0,64;");
      REQUIRE(actors::portraitEntrance(2) ==
              "M0,0,1600;M-310,0,580;M0,0,1250;M320,0,64;");
      REQUIRE(actors::portraitEntrance(3) ==
              "M0,0,2400;M-310,0,580;M0,0,430;M320,0,64;");
      for (int portrait = 1; portrait <= 3; ++portrait) {
        REQUIRE_NOTHROW(parse(actors::portraitEntrance(portrait)));
      }
    }

    THEN("The finale loops its dance and ends the program after 1000 frames") {
      REQUIRE(
          actors::danceFinale() ==
          "A0,(27,10)(28,10)(29,10)(31,10)(29,10)(30,10)(31,10)(32,10)(33,10)("
          "34,"
          "10)($801D,10)($801F,10)($801D,10)($801E,10)($801F,10)($8020,10)($"
          "8021,"
          "10)($8022,10)(34,10)(35,10)(36,10)(37,10)(38,10)(40,10)(41,10)($"
          "8028,"
          "10)($8029,10)(40,10)(41,10)($8028,10)($8029,10)(40,10)(41,10)($8028,"
          "10)($8029,10)(40,10)(41,10)($8028,10)($8029,10)(42,10)(38,10)(39,10)"
          "("
          "35,10)(34,10)(24,10)(25,10)(26,10)(25,10)(24,10)(17,10)($8018,10)($"
          "8019,10)($801A,10)($8019,10)($8018,10)($8011,10);M0,0,1000;");
      REQUIRE_NOTHROW(parse(actors::danceFinale()));
    }

    THEN("The portraits then shuttle 448 px each way from staggered starts") {
      REQUIRE(actors::portraitShuttle(1) ==
              "LX=400;M-528,0,528;A:M448,0,448;M-448,0,448;JA;");
      REQUIRE(actors::portraitShuttle(2) ==
              "LX=550;M-678,0,678;A:M448,0,448;M-448,0,448;JA;");
      REQUIRE(actors::portraitShuttle(3) ==
              "LX=700;M-828,0,828;A:M448,0,448;M-448,0,448;JA;");
      for (int portrait = 1; portrait <= 3; ++portrait) {
        REQUIRE_NOTHROW(parse(actors::portraitShuttle(portrait)));
      }
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
