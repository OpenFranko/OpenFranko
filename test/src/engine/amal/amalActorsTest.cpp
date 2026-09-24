#include "../../../../src/engine/amal/Actors.h"
#include "../../../../src/engine/amal/Program.h"
#include <catch2/catch_all.hpp>
#include <string>

using namespace openfranko::src::engine::amal;

namespace {

bool contains(const std::string &program, const std::string &part) {
  return program.find(part) != std::string::npos;
}

} // namespace

SCENARIO("Hex$ and AMOS booleans splice numbers into the programs") {
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

SCENARIO("FRAN clamps the player to the stage's arena") {
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

SCENARIO("The small actors") {
  THEN("MARTWY only pauses, and the arrow blinks the indicator") {
    REQUIRE(actors::idle() == "A:P;JA;");
    REQUIRE(actors::indicatorArrow(0) == "A0,(1+$0,10)(10,10);");
    REQUIRE_NOTHROW(parse(actors::playerBlood()));
    REQUIRE_NOTHROW(parse(actors::enemyBlood()));
    REQUIRE_NOTHROW(parse(actors::screenShake()));
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

SCENARIO("The spectator, the speech bubbles and the finishing moves") {
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

SCENARIO("KONBOSS on stage 2 throws the boss overhead") {
  THEN("The boss's and the player's scripts are the source's, and parse") {
    REQUIRE(actors::bossThrown() ==
            "M0,0,100;LA=78+RR;M0,0,50;LA=79+RR;M0,0,20;LA=80+RR;M0,0,30;LA=77+"
            "RR;");
    REQUIRE(actors::victoryLift() ==
            "LA=38+RR;M0,0,50;LA=39+RR;M0,0,50;LA=86+RR+RT;M0,0,50;LA=87+RR+"
            "RT;M0,0,50;LA=38+RR;");
    REQUIRE_NOTHROW(parse(actors::bossThrown()));
    REQUIRE_NOTHROW(parse(actors::victoryLift()));
  }
}

SCENARIO("The bonus drive builds each pedestrian from its first image") {
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

SCENARIO("RACZKA's pointing hand waits on R1 and waggles four times") {
  THEN("The program is the source's, and it parses") {
    REQUIRE(actors::pointingHand() ==
            "A:P;IR1=0JA;FR0=0T3;M4,0,2;M-4,0,2;NR0;LR1=0;JA;");
    REQUIRE_NOTHROW(parse(actors::pointingHand()));
  }
}
