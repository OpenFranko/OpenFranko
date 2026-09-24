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
