#include "../../../../../src/engine/states/shared/MusicFadeOut.h"

#include "../../../systems/audio/FakeSpeaker.h"

#include <catch2/catch_all.hpp>

#include <vector>

using namespace openfranko::src::engine::states::shared;
using namespace openfranko::test::src::systems::audio;

SCENARIO("The music fades out a volume level a frame, then stops") {
  GIVEN("Music at full volume") {
    FakeSpeaker speaker;
    MusicFadeOut fade;

    THEN("It takes 64 frames to reach silence and one more to stop") {
      std::vector<int> expected;
      for (int volume = 63; volume >= 0; --volume) {
        REQUIRE_FALSE(fade.advance(speaker));
        expected.push_back(volume);
      }
      REQUIRE(speaker.volumes == expected);
      REQUIRE(speaker.musicStops == 0);
      REQUIRE(fade.advance(speaker));
      REQUIRE(speaker.musicStops == 1);
      REQUIRE(speaker.volumes.size() == 64);
    }
  }
}
