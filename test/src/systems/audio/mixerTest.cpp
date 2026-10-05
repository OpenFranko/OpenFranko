#include "../../../../src/systems/audio/Mixer.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <utility>
#include <vector>

using namespace openfranko::src::systems::audio;

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

std::vector<char> toneModule() {
  std::vector<char> data = tempoModule();
  data.resize(0xC0);
  putWord(data, 0x70 + 0x0E, 0x110 / 16);
  data[0x70 + 0x10] = 16;
  data[0x70 + 0x18] = 16;
  data[0x70 + 0x1C] = 64;
  data[0x70 + 0x1F] = 1;
  putWord(data, 0x70 + 0x20, 8363);
  const std::vector<char> rowZero = {0x20, 0x40, 1, 0};
  std::vector<char> pattern(2, 0);
  pattern.insert(pattern.end(), rowZero.begin(), rowZero.end());
  pattern.insert(pattern.end(), 63, 0);
  putWord(pattern, 0, static_cast<int>(pattern.size()));
  data.insert(data.end(), pattern.begin(), pattern.end());
  data.resize(0x110);
  for (int frame = 0; frame < 16; ++frame) {
    data.push_back(static_cast<char>(frame % 8 < 4 ? 228 : 28));
  }
  return data;
}

std::size_t levels(const std::vector<int16_t> &samples) {
  std::vector<int16_t> sorted(samples);
  std::sort(sorted.begin(), sorted.end());
  return static_cast<std::size_t>(std::unique(sorted.begin(), sorted.end()) -
                                  sorted.begin());
}

void renderSeconds(Mixer &mixer, double seconds) {
  std::vector<int16_t> stereo(static_cast<std::size_t>(MODULE_RATE) * 2);
  for (int frames = static_cast<int>(seconds * MODULE_RATE); frames > 0;
       frames -= MODULE_RATE) {
    mixer.render(stereo.data(), std::min(frames, MODULE_RATE));
  }
}

} // namespace

SCENARIO("A module loop-back does not clear the override") {
  GIVEN("A tune whose single pattern sets tempo 20 on its first row") {
    Mixer mixer(MODULE_RATE);
    REQUIRE(mixer.loadModule(tempoModule()));
    mixer.startModule();
    mixer.setModuleTempo(1.0);
    renderSeconds(mixer, 0.05);

    WHEN("Tempo 14 is set and the module wraps back to an earlier row") {
      mixer.overrideModuleTempo(14);

      THEN("The override stays active past the loop-back") {
        renderSeconds(mixer, 9.5);
        REQUIRE(mixer.isModuleTempoOverridden());
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

SCENARIO("Music can be mixed without interpolation") {
  GIVEN("A tune playing a square wave at a rate that is not the mixer's") {
    const auto play = [](bool interpolation) {
      Mixer mixer(MODULE_RATE);
      REQUIRE(mixer.loadModule(toneModule()));
      mixer.setInterpolation(interpolation);
      mixer.startModule();
      mixer.setModuleTempo(1.0);
      render(mixer, 400);
      return render(mixer, 400).left;
    };

    WHEN("It is mixed with and without interpolation") {
      const std::vector<int16_t> smooth = play(true);
      const std::vector<int16_t> held = play(false);

      THEN("Without interpolation only the wave's own two levels are heard") {
        REQUIRE(levels(held) <= 2);
        REQUIRE(levels(smooth) > 2);
      }
    }
  }
}

namespace {

constexpr int TICK_FRAMES = MODULE_RATE * 5 / 2 / 125;
constexpr int TUNE_FRAMES = 64 * 6 * TICK_FRAMES;
constexpr char SET_SPEED = 1;
constexpr char SET_TEMPO = 20;
constexpr char C4 = 0x40;
constexpr char NOTE = 0x20;
constexpr char VOLUME = 0x40;
constexpr char COMMAND = static_cast<char>(0x80);

using Rows = std::vector<std::pair<int, std::vector<char>>>;

std::vector<char> note(int channel) {
  return {static_cast<char>(NOTE | channel), C4, 1};
}

std::vector<char> command(int channel, char effect, char parameter) {
  return {static_cast<char>(COMMAND | channel), effect, parameter};
}

std::vector<char> pattern(const Rows &rows) {
  std::vector<char> packed;
  for (int row = 0; row < 64; ++row) {
    for (const auto &[at, event] : rows) {
      if (at == row) {
        packed.insert(packed.end(), event.begin(), event.end());
      }
    }
    packed.push_back(0);
  }
  return packed;
}

void align(std::vector<char> &data) {
  data.resize((data.size() + 15) / 16 * 16);
}

std::vector<char> squareWave() {
  std::vector<char> wave;
  for (int frame = 0; frame < 16; ++frame) {
    wave.push_back(static_cast<char>(frame % 8 < 4 ? 228 : 28));
  }
  return wave;
}

std::vector<char> s3mModule(const std::vector<uint8_t> &orders,
                            const std::vector<std::vector<char>> &patterns,
                            const std::vector<char> &sample, bool looped) {
  std::vector<char> data(0x60, 0);
  data[0x1C] = 0x1A;
  data[0x1D] = 16;
  putWord(data, 0x20, static_cast<int>(orders.size()));
  putWord(data, 0x22, 1);
  putWord(data, 0x24, static_cast<int>(patterns.size()));
  putWord(data, 0x28, 0x1320);
  putWord(data, 0x2A, 2);
  data[0x2C] = 'S';
  data[0x2D] = 'C';
  data[0x2E] = 'R';
  data[0x2F] = 'M';
  data[0x30] = 64;
  data[0x31] = 6;
  data[0x32] = 125;
  data[0x33] = static_cast<char>(0xB0);
  for (std::size_t channel = 0; channel < 32; ++channel) {
    data[0x40 + channel] = static_cast<char>(channel < 4 ? channel : 0xFF);
  }
  data.insert(data.end(), orders.begin(), orders.end());
  const std::size_t pointers = data.size();
  data.resize(data.size() + 2 * (1 + patterns.size()));
  align(data);
  const std::size_t header = data.size();
  putWord(data, pointers, static_cast<int>(header / 16));
  data.resize(header + 0x50);
  putWord(data, header + 0x10, static_cast<int>(sample.size()));
  putWord(data, header + 0x18, static_cast<int>(sample.size()));
  data[header] = 1;
  data[header + 0x1C] = 64;
  data[header + 0x1F] = looped ? 1 : 0;
  putWord(data, header + 0x20, 8363);
  data[header + 0x4C] = 'S';
  data[header + 0x4D] = 'C';
  data[header + 0x4E] = 'R';
  data[header + 0x4F] = 'S';
  for (std::size_t index = 0; index < patterns.size(); ++index) {
    if (patterns[index].empty()) {
      continue;
    }
    align(data);
    putWord(data, pointers + 2 * (1 + index),
            static_cast<int>(data.size() / 16));
    std::vector<char> packed(2, 0);
    packed.insert(packed.end(), patterns[index].begin(), patterns[index].end());
    putWord(packed, 0, static_cast<int>(packed.size()));
    data.insert(data.end(), packed.begin(), packed.end());
  }
  align(data);
  putWord(data, header + 0x0E, static_cast<int>(data.size() / 16));
  data.insert(data.end(), sample.begin(), sample.end());
  return data;
}

std::vector<char> steadyTone() {
  return s3mModule({0, 255}, {pattern({{0, note(0)}})}, squareWave(), true);
}

std::vector<int16_t> left(Mixer &mixer, int frames) {
  return render(mixer, frames).left;
}

bool silent(const std::vector<int16_t> &samples) {
  return std::all_of(samples.begin(), samples.end(),
                     [](int16_t sample) { return sample == 0; });
}

std::vector<int16_t> playedTone(std::optional<int> volume) {
  Mixer mixer(MODULE_RATE);
  REQUIRE(mixer.loadModule(steadyTone()));
  if (volume) {
    mixer.setMusicVolume(*volume);
  }
  mixer.startModule();
  mixer.setModuleTempo(1.0);
  left(mixer, 400);
  return left(mixer, 400);
}

std::vector<std::size_t> onsets(const std::vector<int16_t> &samples) {
  std::vector<std::size_t> found;
  std::size_t quiet = 1000;
  for (std::size_t at = 0; at < samples.size(); ++at) {
    if (samples[at] == 0) {
      ++quiet;
      continue;
    }
    if (quiet >= 1000) {
      found.push_back(at);
    }
    quiet = 0;
  }
  return found;
}

std::vector<char> protrackerModule() {
  std::vector<char> data(1084, 0);
  data[20 + 23] = 8;
  data[20 + 25] = 64;
  data[20 + 29] = 8;
  data[950] = 1;
  data[951] = 0x7F;
  data[1080] = 'M';
  data[1081] = '.';
  data[1082] = 'K';
  data[1083] = '.';
  std::vector<char> rows(1024, 0);
  rows[0] = 0x01;
  rows[1] = static_cast<char>(0xAC);
  rows[2] = 0x10;
  data.insert(data.end(), rows.begin(), rows.end());
  for (int frame = 0; frame < 16; ++frame) {
    data.push_back(static_cast<char>(frame % 8 < 4 ? 100 : -100));
  }
  return data;
}

} // namespace

SCENARIO("The tune's own speed and tempo commands end an override") {
  GIVEN("A tune that sets speed 5 and tempo 125, then speed 3 on row 16, "
        "with a note and volume on row 4") {
    Mixer mixer(MODULE_RATE);
    const std::vector<char> rowFour = {static_cast<char>(NOTE | VOLUME), C4, 1,
                                       32};
    REQUIRE(
        mixer.loadModule(s3mModule({0, 255},
                                   {pattern({{0, command(0, SET_SPEED, 5)},
                                             {0, command(1, SET_TEMPO, 125)},
                                             {4, rowFour},
                                             {16, command(0, SET_SPEED, 3)}})},
                                   squareWave(), true)));
    mixer.startModule();
    mixer.setModuleTempo(1.0);
    renderSeconds(mixer, 0.05);

    WHEN("Tempo 14 is set on its first row") {
      mixer.overrideModuleTempo(14);
      renderSeconds(mixer, 2.15);
      const bool beforeRow16 = mixer.isModuleTempoOverridden();
      renderSeconds(mixer, 0.15);

      THEN("The override lasts until row 16 sets the speed again") {
        REQUIRE(beforeRow16);
        REQUIRE_FALSE(mixer.isModuleTempoOverridden());
        REQUIRE(mixer.isModulePlaying());
      }
    }
  }

  GIVEN("A tune whose first pattern is stored nowhere and whose second sets "
        "the speed on its first row") {
    Mixer mixer(MODULE_RATE);
    REQUIRE(mixer.loadModule(
        s3mModule({0, 1, 255}, {{}, pattern({{0, command(0, SET_SPEED, 6)}})},
                  squareWave(), true)));
    mixer.startModule();
    mixer.setModuleTempo(1.0);
    mixer.overrideModuleTempo(14);

    THEN("The empty pattern plays out under the override and the second "
         "ends it") {
      renderSeconds(mixer, 8.9);
      REQUIRE(mixer.isModuleTempoOverridden());
      renderSeconds(mixer, 0.5);
      REQUIRE_FALSE(mixer.isModuleTempoOverridden());
    }
  }
}

SCENARIO("An override follows the tune's speed when the tune loops back") {
  GIVEN("A tune at speed 5 with notes on rows 0 and 31 that slows to speed 3 "
        "on row 32") {
    Mixer mixer(MODULE_RATE);
    const std::vector<char> blip(64, static_cast<char>(228));
    REQUIRE(
        mixer.loadModule(s3mModule({0, 255},
                                   {pattern({{0, note(0)},
                                             {0, command(1, SET_SPEED, 5)},
                                             {0, command(2, SET_TEMPO, 125)},
                                             {31, note(0)},
                                             {32, command(1, SET_SPEED, 3)}})},
                                   blip, false)));
    mixer.startModule();
    mixer.setModuleTempo(1.0);
    renderSeconds(mixer, 3.5);

    WHEN("Tempo 14 is set after row 32 and the tune comes round again") {
      mixer.overrideModuleTempo(14);
      std::vector<int16_t> heard;
      for (int second = 0; second < 10; ++second) {
        const std::vector<int16_t> part = left(mixer, MODULE_RATE);
        heard.insert(heard.end(), part.begin(), part.end());
      }
      const std::vector<std::size_t> notes = onsets(heard);

      THEN("Rows 0 to 31 of the second pass take Tempo 14's 1/7 s each") {
        REQUIRE(notes.size() >= 2);
        const double rows = static_cast<double>(notes[1] - notes[0]) /
                            MODULE_RATE / (2.0 / 14.0);
        REQUIRE(rows == Catch::Approx(31.0).margin(0.4));
      }
    }
  }
}

SCENARIO("A tune played once ends cleanly however the output is cut") {
  GIVEN("A steady tone over one pattern at speed 6 and tempo 125") {
    THEN("It sounds to the end of its last tick, then stops") {
      for (const int chunk : {TICK_FRAMES, 100}) {
        CAPTURE(chunk);
        Mixer mixer(MODULE_RATE);
        REQUIRE(mixer.loadModule(steadyTone()));
        mixer.startModule(false);
        mixer.setModuleTempo(1.0);
        std::vector<int16_t> heard;
        while (mixer.isModulePlaying() && heard.size() < 2u * TUNE_FRAMES) {
          const std::vector<int16_t> part = left(mixer, chunk);
          heard.insert(heard.end(), part.begin(), part.end());
        }
        REQUIRE_FALSE(mixer.isModulePlaying());
        const auto last =
            std::find_if(heard.rbegin(), heard.rend(),
                         [](int16_t sample) { return sample != 0; });
        const std::size_t end = static_cast<std::size_t>(heard.rend() - last);
        REQUIRE(end <= static_cast<std::size_t>(TUNE_FRAMES));
        REQUIRE(end > static_cast<std::size_t>(TUNE_FRAMES - TICK_FRAMES));
        REQUIRE(heard.size() <= static_cast<std::size_t>(TUNE_FRAMES + chunk));
      }
    }
  }

  GIVEN("The tone played once to its end") {
    Mixer mixer(MODULE_RATE);
    REQUIRE(mixer.loadModule(steadyTone()));
    mixer.startModule(false);
    mixer.setModuleTempo(1.0);
    renderSeconds(mixer, 8.0);
    mixer.update();

    THEN("After the update it can be started again from the beginning") {
      REQUIRE_FALSE(mixer.isModulePlaying());
      mixer.startModule(false);
      mixer.setModuleTempo(1.0);
      REQUIRE(mixer.isModulePlaying());
      REQUIRE_FALSE(silent(left(mixer, 400)));
    }
  }
}

SCENARIO("A tune can be stopped, released and replaced") {
  GIVEN("The tone playing") {
    Mixer mixer(MODULE_RATE);
    REQUIRE(mixer.loadModule(steadyTone()));
    mixer.startModule();
    mixer.setModuleTempo(1.0);
    REQUIRE_FALSE(silent(left(mixer, 400)));

    WHEN("It is stopped") {
      mixer.stopModule();

      THEN("It is silent until started again") {
        REQUIRE_FALSE(mixer.isModulePlaying());
        REQUIRE(silent(left(mixer, 400)));
        mixer.startModule();
        REQUIRE(mixer.isModulePlaying());
        REQUIRE_FALSE(silent(left(mixer, 400)));
      }
    }

    WHEN("It is released") {
      mixer.releaseModule();
      mixer.startModule();

      THEN("There is nothing left to start") {
        REQUIRE_FALSE(mixer.isModulePlaying());
        REQUIRE(silent(left(mixer, 400)));
      }
    }

    WHEN("A tune without notes is loaded in its place") {
      REQUIRE(mixer.loadModule(tempoModule()));
      mixer.startModule();

      THEN("The new tune plays") {
        REQUIRE(mixer.isModulePlaying());
        REQUIRE(silent(left(mixer, 400)));
      }
    }

    WHEN("Data that is no tune is loaded in its place") {
      const bool loaded = mixer.loadModule(std::vector<char>(64, 7));
      mixer.startModule();

      THEN("The old tune is gone too") {
        REQUIRE_FALSE(loaded);
        REQUIRE_FALSE(mixer.isModulePlaying());
      }
    }
  }

  GIVEN("A mixer that never loaded a tune") {
    Mixer mixer(MODULE_RATE);

    THEN("Releasing, starting and setting the tempo do nothing") {
      mixer.releaseModule();
      REQUIRE_FALSE(mixer.loadModule({}));
      mixer.startModule();
      mixer.setModuleTempo(2.0);
      mixer.overrideModuleTempo(14);
      REQUIRE_FALSE(mixer.isModulePlaying());
      REQUIRE_FALSE(mixer.isModuleTempoOverridden());
      REQUIRE(silent(left(mixer, 100)));
    }
  }

  GIVEN("A tune playing") {
    Mixer mixer(MODULE_RATE);
    REQUIRE(mixer.loadModule(steadyTone()));
    mixer.startModule();

    THEN("Tempo 0 and below are no override") {
      mixer.overrideModuleTempo(0);
      REQUIRE_FALSE(mixer.isModuleTempoOverridden());
      mixer.overrideModuleTempo(-5);
      REQUIRE_FALSE(mixer.isModuleTempoOverridden());
    }
  }
}

SCENARIO("The music volume scales the tune and clips past full volume") {
  GIVEN("The tone at the default volume and at 64, 32, 0 and 3200") {
    const std::vector<int16_t> standard = playedTone(std::nullopt);
    const std::vector<int16_t> full = playedTone(64);
    const std::vector<int16_t> half = playedTone(32);
    const std::vector<int16_t> off = playedTone(0);
    const std::vector<int16_t> loud = playedTone(3200);

    THEN("Each sample is the tune's times the volume over 64") {
      REQUIRE_FALSE(silent(full));
      REQUIRE(silent(off));
      bool clipped = false;
      for (std::size_t at = 0; at < full.size(); ++at) {
        REQUIRE(standard[at] == full[at] * 56 / 64);
        REQUIRE(half[at] == full[at] * 32 / 64);
        const long louder = full[at] * 50L;
        REQUIRE(loud[at] == std::clamp(louder, -32768L, 32767L));
        clipped = clipped || louder > 32767 || louder < -32768;
      }
      REQUIRE(clipped);
    }
  }
}

SCENARIO("Interpolation can be switched while the music plays") {
  GIVEN("The square wave tune playing with interpolation") {
    Mixer mixer(MODULE_RATE);
    REQUIRE(mixer.loadModule(toneModule()));
    mixer.startModule();
    mixer.setModuleTempo(1.0);
    left(mixer, 400);

    WHEN("It is switched off and back on") {
      mixer.setInterpolation(false);
      left(mixer, 400);
      const std::vector<int16_t> held = left(mixer, 400);
      mixer.setInterpolation(true);
      left(mixer, 400);
      const std::vector<int16_t> smooth = left(mixer, 400);

      THEN("The wave's own two levels are heard only while it is off") {
        REQUIRE(levels(held) <= 2);
        REQUIRE(levels(smooth) > 2);
      }
    }
  }
}

SCENARIO("Modules other than S3M play too") {
  GIVEN("A ProTracker module with a note on its first row") {
    Mixer mixer(MODULE_RATE);
    REQUIRE(mixer.loadModule(protrackerModule()));

    THEN("It plays like an S3M tune") {
      mixer.startModule();
      mixer.setModuleTempo(1.0);
      REQUIRE(mixer.isModulePlaying());
      REQUIRE_FALSE(silent(left(mixer, 400)));
    }
  }
}
