#include "../../../src/systems/Mixer.h"
#include <catch2/catch_all.hpp>

#include <cstdint>
#include <cstdlib>
#include <vector>

using namespace openfranko::src::systems;

namespace {

constexpr int RATE = 1000;

struct Output {
  std::vector<int16_t> left;
  std::vector<int16_t> right;
};

Output render(Mixer &mixer, int frames) {
  std::vector<int16_t> stereo(static_cast<std::size_t>(frames) * 2);
  mixer.render(stereo.data(), frames);
  Output output;
  for (int frame = 0; frame < frames; ++frame) {
    output.left.push_back(stereo[static_cast<std::size_t>(frame) * 2]);
    output.right.push_back(stereo[static_cast<std::size_t>(frame) * 2 + 1]);
  }
  return output;
}

Sound sound(std::vector<int16_t> frames, int rate = RATE) {
  return Sound{rate, std::move(frames)};
}

} // namespace

SCENARIO("A sample plays on one side at Volume 56") {
  GIVEN("A mixer and a short sample") {
    Mixer mixer(RATE);
    const Sound sample = sound({6400, -6400, 64, 3});

    WHEN("It plays on voice 0") {
      mixer.play(sample, 0x1, 0, false);
      const Output output = render(mixer, 5);

      THEN("It sounds on the left at 56/64, rounded toward zero") {
        REQUIRE(output.left == std::vector<int16_t>{5600, -5600, 56, 2, 0});
        REQUIRE(output.right == std::vector<int16_t>(5, 0));
      }

      THEN("The voice is free once the sample has ended") {
        REQUIRE_FALSE(mixer.isPlaying(0));
      }
    }

    WHEN("It plays on each voice in turn") {
      THEN("Voices 0 and 3 are left, 1 and 2 are right") {
        for (int voice = 0; voice < Mixer::VOICES; ++voice) {
          mixer.play(sample, 1 << voice, 0, false);
          const Output output = render(mixer, 1);
          const bool left = voice == 0 || voice == 3;
          REQUIRE(output.left[0] == (left ? 5600 : 0));
          REQUIRE(output.right[0] == (left ? 0 : 5600));
          render(mixer, 4);
        }
      }
    }
  }

  GIVEN("A loud sample on both left voices") {
    Mixer mixer(RATE);
    const Sound loud = sound({32767, -32768});
    mixer.play(loud, 0x9, 0, false);

    THEN("The sum clips to 16 bits") {
      const Output output = render(mixer, 2);
      REQUIRE(output.left == std::vector<int16_t>{32767, -32768});
    }
  }
}

SCENARIO("A sample steps through its frames at its playing rate") {
  GIVEN("A mixer and a four-frame sample") {
    Mixer mixer(RATE);
    const Sound sample = sound({640, 1280, 1920, 2560});

    THEN("Half the output rate holds every frame for two output frames") {
      mixer.play(sample, 0x1, RATE / 2, false);
      REQUIRE(render(mixer, 9).left == std::vector<int16_t>{560, 560, 1120,
                                                            1120, 1680, 1680,
                                                            2240, 2240, 0});
    }

    THEN("Twice the output rate skips every other frame") {
      mixer.play(sample, 0x1, RATE * 2, false);
      REQUIRE(render(mixer, 3).left == std::vector<int16_t>{560, 1680, 0});
    }

    THEN("Frequency 0 plays at the sample's own rate") {
      const Sound slow = sound({640, 1280}, RATE / 2);
      mixer.play(slow, 0x1, 0, false);
      REQUIRE(render(mixer, 5).left ==
              std::vector<int16_t>{560, 560, 1120, 1120, 0});
    }
  }
}

SCENARIO("A looping sample restarts until its loops are ended") {
  GIVEN("A two-frame sample playing in a loop") {
    Mixer mixer(RATE);
    const Sound sample = sound({640, 1280});
    mixer.play(sample, 0x1, 0, true);

    THEN("It starts over at its end") {
      REQUIRE(render(mixer, 5).left ==
              std::vector<int16_t>{560, 1120, 560, 1120, 560});
    }

    WHEN("Its loop is ended midway") {
      render(mixer, 3);
      mixer.endLoops();

      THEN("It finishes the pass it is on and stops") {
        REQUIRE(render(mixer, 3).left == std::vector<int16_t>{1120, 0, 0});
        REQUIRE_FALSE(mixer.isPlaying(0));
      }
    }
  }
}

SCENARIO("A sample on all four voices silences the music") {
  GIVEN("A mixer") {
    Mixer mixer(RATE);
    const Sound sample = sound({640, 1280});

    THEN("Three voices leave the music alone") {
      mixer.play(sample, 0x7, 0, false);
      REQUIRE_FALSE(mixer.isMusicSilenced());
    }

    WHEN("The sample plays on all four voices") {
      mixer.play(sample, Mixer::ALL_VOICES, 0, false);

      THEN("The music is silenced") { REQUIRE(mixer.isMusicSilenced()); }

      AND_WHEN("The sample ends") {
        render(mixer, 3);

        THEN("The music comes back at the next update") {
          REQUIRE(mixer.isMusicSilenced());
          mixer.update();
          REQUIRE_FALSE(mixer.isMusicSilenced());
        }
      }

      AND_WHEN("It is still playing at the update") {
        render(mixer, 1);
        mixer.update();

        THEN("The music stays silenced") { REQUIRE(mixer.isMusicSilenced()); }
      }

      AND_WHEN("The sample is stopped") {
        mixer.stop(sample);

        THEN("Its voices are free and the music comes back at once") {
          for (int voice = 0; voice < Mixer::VOICES; ++voice) {
            REQUIRE_FALSE(mixer.isPlaying(voice));
          }
          REQUIRE_FALSE(mixer.isMusicSilenced());
        }
      }
    }
  }
}

SCENARIO("stopAll frees every voice") {
  GIVEN("A looping sample on every voice") {
    Mixer mixer(RATE);
    const Sound sample = sound({640, 1280});
    mixer.play(sample, Mixer::ALL_VOICES, 0, true);

    THEN("Nothing plays after stopAll") {
      mixer.stopAll();
      REQUIRE(render(mixer, 2).left == std::vector<int16_t>{0, 0});
    }
  }
}

SCENARIO("The LED filter smooths the output") {
  GIVEN("A mixer at the game's rate and a steady sample") {
    constexpr int GAME_RATE = 22050;
    Mixer mixer(GAME_RATE);
    const Sound steady = sound(std::vector<int16_t>(64, 16000), GAME_RATE);

    THEN("With the filter off the output is the sample") {
      mixer.play(steady, 0x1, 0, false);
      const Output output = render(mixer, 64);
      REQUIRE(output.left.front() == 14000);
      REQUIRE(output.left.back() == 14000);
    }

    THEN("With the filter on the step rises smoothly to the same level") {
      mixer.setFilter(true);
      mixer.play(steady, 0x1, 0, false);
      const Output output = render(mixer, 64);
      REQUIRE(output.left.front() > 0);
      REQUIRE(output.left.front() < 14000);
      REQUIRE(std::abs(output.left.back() - 14000) <= 1);
    }
  }
}
