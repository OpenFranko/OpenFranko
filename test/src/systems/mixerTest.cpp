#include "../../../src/systems/Mixer.h"
#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
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

Sound sound(std::vector<int8_t> frames, int rate = RATE) {
  return Sound{rate, std::move(frames)};
}

} // namespace

SCENARIO("A sample plays on one side at Volume 56") {
  GIVEN("A mixer and a short sample") {
    Mixer mixer(RATE);
    const Sound sample = sound({25, -25, 1, 0});

    WHEN("It plays on voice 0") {
      mixer.play(sample, 0x1, 0, false);
      const Output output = render(mixer, 5);

      THEN("It sounds on the left at 56/64 of half the 16-bit range") {
        REQUIRE(output.left == std::vector<int16_t>{2800, -2800, 112, 0, 0});
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
          REQUIRE(output.left[0] == (left ? 2800 : 0));
          REQUIRE(output.right[0] == (left ? 0 : 2800));
          render(mixer, 4);
        }
      }
    }
  }

  GIVEN("A loud sample on both left voices") {
    Mixer mixer(RATE);
    const Sound loud = sound({127, -128});
    mixer.play(loud, 0x9, 0, false);

    THEN("The sum fits in 16 bits without clipping") {
      const Output output = render(mixer, 2);
      REQUIRE(output.left == std::vector<int16_t>{28448, -28672});
    }
  }
}

SCENARIO("A sample steps through its frames at its playing rate") {
  GIVEN("A mixer and a four-frame sample") {
    Mixer mixer(RATE);
    const Sound sample = sound({10, 20, 30, 40});

    THEN("Half the output rate holds every frame for two output frames") {
      mixer.play(sample, 0x1, RATE / 2, false);
      REQUIRE(render(mixer, 9).left == std::vector<int16_t>{1120, 1120, 2240,
                                                            2240, 3360, 3360,
                                                            4480, 4480, 0});
    }

    THEN("Twice the output rate skips every other frame") {
      mixer.play(sample, 0x1, RATE * 2, false);
      REQUIRE(render(mixer, 3).left == std::vector<int16_t>{1120, 3360, 0});
    }

    THEN("Frequency 0 plays at the sample's own rate") {
      const Sound slow = sound({10, 20}, RATE / 2);
      mixer.play(slow, 0x1, 0, false);
      REQUIRE(render(mixer, 5).left ==
              std::vector<int16_t>{1120, 1120, 2240, 2240, 0});
    }
  }
}

SCENARIO("A looping sample restarts until its loops are ended") {
  GIVEN("A two-frame sample playing in a loop") {
    Mixer mixer(RATE);
    const Sound sample = sound({10, 20});
    mixer.play(sample, 0x1, 0, true);

    THEN("It starts over at its end") {
      REQUIRE(render(mixer, 5).left ==
              std::vector<int16_t>{1120, 2240, 1120, 2240, 1120});
    }

    WHEN("Its loop is ended midway") {
      render(mixer, 3);
      mixer.endLoops();

      THEN("It finishes the pass it is on and stops") {
        REQUIRE(render(mixer, 3).left == std::vector<int16_t>{2240, 0, 0});
        REQUIRE_FALSE(mixer.isPlaying(0));
      }
    }
  }
}

SCENARIO("A sample on all four voices silences the music") {
  GIVEN("A mixer") {
    Mixer mixer(RATE);
    const Sound sample = sound({10, 20});

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
    const Sound sample = sound({10, 20});
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
    const Sound steady = sound(std::vector<int8_t>(64, 64), GAME_RATE);

    THEN("With the filter off the output is the sample") {
      mixer.play(steady, 0x1, 0, false);
      const Output output = render(mixer, 64);
      REQUIRE(output.left.front() == 7168);
      REQUIRE(output.left.back() == 7168);
    }

    THEN("With the filter on the step rises smoothly to the same level") {
      mixer.setFilter(true);
      mixer.play(steady, 0x1, 0, false);
      const Output output = render(mixer, 64);
      REQUIRE(output.left.front() > 0);
      REQUIRE(output.left.front() < 7168);
      REQUIRE(std::abs(output.left.back() - 7168) <= 1);
    }
  }
}

namespace {

constexpr int MODULE_RATE = 8000;

void putWord(std::vector<char> &data, std::size_t at, int value) {
  data[at] = static_cast<char>(value & 0xFF);
  data[at + 1] = static_cast<char>(value >> 8);
}

std::vector<char> tempoModule() {
  std::vector<char> data(0xC0, 0);
  data[0x1C] = 0x1A;
  data[0x1D] = 16;
  putWord(data, 0x20, 2);
  putWord(data, 0x22, 1);
  putWord(data, 0x24, 1);
  putWord(data, 0x28, 0x1320);
  putWord(data, 0x2A, 2);
  data[0x2C] = 'S';
  data[0x2D] = 'C';
  data[0x2E] = 'R';
  data[0x2F] = 'M';
  data[0x30] = 64;
  data[0x31] = 6;
  data[0x32] = static_cast<char>(128);
  data[0x33] = static_cast<char>(0xB0);
  for (int channel = 0; channel < 32; ++channel) {
    data[0x40 + static_cast<std::size_t>(channel)] =
        static_cast<char>(channel < 4 ? channel : 0xFF);
  }
  data[0x60] = 0;
  data[0x61] = static_cast<char>(0xFF);
  putWord(data, 0x62, 0x70 / 16);
  putWord(data, 0x64, 0xC0 / 16);
  data[0x70] = 1;
  data[0x70 + 0x4C] = 'S';
  data[0x70 + 0x4D] = 'C';
  data[0x70 + 0x4E] = 'R';
  data[0x70 + 0x4F] = 'S';
  const std::vector<char> rowZero = {
      static_cast<char>(0x80), 1, 5, static_cast<char>(0x81), 20,
      static_cast<char>(125),  0};
  std::vector<char> pattern(2, 0);
  pattern.insert(pattern.end(), rowZero.begin(), rowZero.end());
  pattern.insert(pattern.end(), 63, 0);
  putWord(pattern, 0, static_cast<int>(pattern.size()));
  data.insert(data.end(), pattern.begin(), pattern.end());
  return data;
}

void renderSeconds(Mixer &mixer, double seconds) {
  std::vector<int16_t> stereo(static_cast<std::size_t>(MODULE_RATE) * 2);
  for (int frames = static_cast<int>(seconds * MODULE_RATE); frames > 0;
       frames -= MODULE_RATE) {
    mixer.render(stereo.data(), std::min(frames, MODULE_RATE));
  }
}

} // namespace

SCENARIO("Tempo holds until the tune's own tempo command comes round again") {
  GIVEN("A tune whose single pattern sets tempo 20 on its first row") {
    Mixer mixer(MODULE_RATE);
    REQUIRE(mixer.loadModule(tempoModule()));
    mixer.startModule();
    mixer.setModuleTempo(1.0);
    renderSeconds(mixer, 0.05);

    WHEN("Tempo 14 is set on the first row") {
      mixer.overrideModuleTempo(14);

      THEN("It still holds after the 6.4 s the pattern lasts at tempo 20") {
        renderSeconds(mixer, 8.5);
        REQUIRE(mixer.isModuleTempoOverridden());
      }

      THEN("The first row's command ends it once the 9.1 s pattern loops") {
        renderSeconds(mixer, 9.5);
        REQUIRE_FALSE(mixer.isModuleTempoOverridden());
      }
    }

    WHEN("The music is restarted") {
      mixer.overrideModuleTempo(14);
      mixer.startModule();

      THEN("The override is gone") {
        REQUIRE_FALSE(mixer.isModuleTempoOverridden());
      }
    }
  }
}

SCENARIO("Tempo set before the tune's first row has sounded still holds") {
  GIVEN("A tune whose first row sets tempo 20, not yet rendered") {
    Mixer mixer(MODULE_RATE);
    REQUIRE(mixer.loadModule(tempoModule()));
    mixer.startModule();
    mixer.setModuleTempo(1.0);

    WHEN("Tempo 14 is set at once") {
      mixer.overrideModuleTempo(14);

      THEN("The first row's own command does not change the pace") {
        renderSeconds(mixer, 8.5);
        REQUIRE(mixer.isModuleTempoOverridden());
        renderSeconds(mixer, 1.0);
        REQUIRE_FALSE(mixer.isModuleTempoOverridden());
      }
    }
  }
}

SCENARIO("A tune can be played once instead of looping") {
  GIVEN("The 6.4 s tune") {
    Mixer mixer(MODULE_RATE);
    REQUIRE(mixer.loadModule(tempoModule()));

    WHEN("It is started to play once") {
      mixer.startModule(false);
      mixer.setModuleTempo(1.0);

      THEN("It plays to its end and then stops") {
        renderSeconds(mixer, 6.2);
        REQUIRE(mixer.isModulePlaying());
        renderSeconds(mixer, 0.4);
        REQUIRE_FALSE(mixer.isModulePlaying());
      }
    }

    WHEN("It is started as usual") {
      mixer.startModule();
      mixer.setModuleTempo(1.0);

      THEN("It keeps looping") {
        renderSeconds(mixer, 13.0);
        REQUIRE(mixer.isModulePlaying());
      }
    }
  }
}
