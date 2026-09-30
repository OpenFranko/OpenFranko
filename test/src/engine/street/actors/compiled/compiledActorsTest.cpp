#include "../../../../../../src/engine/street/actors/compiled/CompiledActors.h"

#include "../../../../../../src/engine/amal/Program.h"
#include "../../../../../../src/engine/street/actors/Actors.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::amal;
using namespace openfranko::src::engine::street;

namespace {

constexpr auto REGENERATE = "Regenerate with compileActorPrograms -o "
                            "src/engine/street/actors/compiled/"
                            "CompiledActors.cpp";

bool sameTerms(const Program &program, const Expression &compiled,
               const ParsedProgram &parsed, const Expression &source) {
  if (compiled.terms != source.terms) {
    return false;
  }
  for (uint16_t i = 0; i < compiled.terms; ++i) {
    const Term &a = program.terms[compiled.first + i];
    const Term &b = parsed.terms[source.first + i];
    if (a.kind != b.kind || a.op != b.op || a.value != b.value) {
      return false;
    }
  }
  return true;
}

bool isCompiledFrom(const Program &program, const std::string &source) {
  const ParsedProgram parsed = parse(source);
  if (program.length != static_cast<int16_t>(parsed.instructions.size())) {
    return false;
  }
  for (int16_t pc = 0; pc < program.length; ++pc) {
    const Instruction &a = program.instructions[program.code[pc]];
    const Instruction &b = parsed.instructions[parsed.code[pc]];
    if (a.opcode != b.opcode || a.reg != b.reg || a.jump != b.jump ||
        a.frames != b.frames || !sameTerms(program, a.first, parsed, b.first) ||
        !sameTerms(program, a.second, parsed, b.second) ||
        !sameTerms(program, a.third, parsed, b.third)) {
      return false;
    }
    for (uint16_t i = 0; i < a.frames; ++i) {
      const AnimFrame &x = program.frames[a.firstFrame + i];
      const AnimFrame &y = parsed.frames[b.firstFrame + i];
      if (!sameTerms(program, x.image, parsed, y.image) ||
          !sameTerms(program, x.delay, parsed, y.delay)) {
        return false;
      }
    }
  }
  return true;
}

} // namespace

SCENARIO("The compiled actor programs are their AMAL sources compiled") {
  GIVEN("Every variant the stages and scenes create") {
    INFO(REGENERATE);

    THEN("The programs that follow the game version match") {
      for (const GameVersion version : actors::compiled::VERSIONS) {
        CAPTURE(static_cast<int>(version));
        REQUIRE(isCompiledFrom(actors::compiled::enemyBlood(version),
                               actors::enemyBlood(version)));
        REQUIRE(isCompiledFrom(actors::compiled::screenShake(version),
                               actors::screenShake(version)));
        REQUIRE(isCompiledFrom(actors::compiled::idle(version),
                               actors::idle(version)));
        for (const int stage : actors::compiled::STAGES) {
          CAPTURE(stage);
          const auto compiled = actors::compiled::streetPlayer(stage, version);
          const auto source = actors::streetPlayer(stage, version);
          REQUIRE(isCompiledFrom(compiled.locomotion, source.locomotion));
          REQUIRE(isCompiledFrom(compiled.damage, source.damage));
          REQUIRE(isCompiledFrom(compiled.clamp, source.clamp));
        }
      }
    }

    THEN("Every enemy image base and type matches in both versions") {
      for (const int base : actors::compiled::ENEMY_IMAGE_BASES) {
        for (const int type : actors::compiled::ENEMY_TYPES) {
          for (const GameVersion version : actors::compiled::VERSIONS) {
            CAPTURE(base, type, static_cast<int>(version));
            const auto compiled = actors::compiled::enemy(base, type, version);
            const auto source = actors::enemy(base, type, version);
            REQUIRE(isCompiledFrom(compiled.walk, source.walk));
            REQUIRE(isCompiledFrom(compiled.damage, source.damage));
          }
        }
      }
    }

    THEN("The boss stages match on every stage") {
      for (const int stage : actors::compiled::STAGES) {
        CAPTURE(stage);
        const auto player = actors::compiled::bossPlayer(stage);
        const auto playerSource = actors::bossPlayer(stage);
        REQUIRE(isCompiledFrom(player.locomotion, playerSource.locomotion));
        REQUIRE(isCompiledFrom(player.damage, playerSource.damage));
        REQUIRE(isCompiledFrom(player.clamp, playerSource.clamp));
        const auto boss = actors::compiled::boss(stage);
        const auto bossSource = actors::boss(stage);
        REQUIRE(isCompiledFrom(boss.walk, bossSource.walk));
        REQUIRE(isCompiledFrom(boss.damage, bossSource.damage));
        REQUIRE(isCompiledFrom(actors::compiled::spectator(stage),
                               actors::spectator(stage)));
        const auto dialogue = actors::compiled::dialogue(stage);
        const auto dialogueSource = actors::dialogue(stage);
        REQUIRE(isCompiledFrom(dialogue.player, dialogueSource.player));
        REQUIRE(isCompiledFrom(dialogue.boss, dialogueSource.boss));
      }
      REQUIRE(isCompiledFrom(actors::compiled::playerBlood(),
                             actors::playerBlood()));
      REQUIRE(
          isCompiledFrom(actors::compiled::walkToBoss(), actors::walkToBoss()));
      REQUIRE(isCompiledFrom(actors::compiled::finishingPose(),
                             actors::finishingPose()));
      REQUIRE(isCompiledFrom(actors::compiled::finishingBlood(),
                             actors::finishingBlood()));
      REQUIRE(isCompiledFrom(actors::compiled::finishingPoseBack(),
                             actors::finishingPoseBack()));
      REQUIRE(isCompiledFrom(actors::compiled::walkOff(), actors::walkOff()));
      REQUIRE(
          isCompiledFrom(actors::compiled::bossThrown(), actors::bossThrown()));
      REQUIRE(isCompiledFrom(actors::compiled::victoryLift(),
                             actors::victoryLift()));
      REQUIRE(
          isCompiledFrom(actors::compiled::bossRests(), actors::bossRests()));
      REQUIRE(isCompiledFrom(actors::compiled::bubbleUntilFire(),
                             actors::bubbleUntilFire()));
    }

    THEN("The arrow, the car stage and the scenes match") {
      for (const int facing : actors::compiled::ARROW_FACINGS) {
        CAPTURE(facing);
        REQUIRE(isCompiledFrom(actors::compiled::indicatorArrow(facing),
                               actors::indicatorArrow(facing)));
      }
      for (const int image : actors::compiled::PEDESTRIAN_IMAGES) {
        CAPTURE(image);
        REQUIRE(isCompiledFrom(actors::compiled::pedestrian(image),
                               actors::pedestrian(image)));
      }
      for (const int portrait : actors::compiled::PORTRAITS) {
        CAPTURE(portrait);
        REQUIRE(isCompiledFrom(actors::compiled::portraitEntrance(portrait),
                               actors::portraitEntrance(portrait)));
        REQUIRE(isCompiledFrom(actors::compiled::portraitShuttle(portrait),
                               actors::portraitShuttle(portrait)));
      }
      REQUIRE(isCompiledFrom(actors::compiled::carDriveOff(),
                             actors::carDriveOff()));
      REQUIRE(isCompiledFrom(actors::compiled::pointingHand(),
                             actors::pointingHand()));
      REQUIRE(isCompiledFrom(actors::compiled::walkAway(), actors::walkAway()));
      REQUIRE(
          isCompiledFrom(actors::compiled::breakDance(), actors::breakDance()));
      REQUIRE(isCompiledFrom(actors::compiled::danceFinale(),
                             actors::danceFinale()));
    }
  }

  GIVEN("Values no stage passes") {
    THEN("The compiled tables refuse them") {
      REQUIRE_THROWS_AS(actors::compiled::enemy(10, 0), std::out_of_range);
      REQUIRE_THROWS_AS(actors::compiled::enemy(0, 3), std::out_of_range);
      REQUIRE_THROWS_AS(actors::compiled::streetPlayer(4), std::out_of_range);
      REQUIRE_THROWS_AS(actors::compiled::indicatorArrow(-32768),
                        std::out_of_range);
      REQUIRE_THROWS_AS(actors::compiled::pedestrian(10), std::out_of_range);
    }
  }
}
