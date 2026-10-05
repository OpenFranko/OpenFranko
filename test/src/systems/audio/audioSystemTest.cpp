#include "../../../../src/systems/audio/AudioSystem.h"

#include "../HeadlessSdl.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace openfranko::src::systems::audio;
using namespace openfranko::test::src::systems;

namespace {

constexpr auto TUNE = "assets/0259.s3m";
constexpr auto NOISE = "assets/025A.s3m";
constexpr auto MISSING_TUNE = "assets/025B.s3m";
constexpr auto SAMPLE = "assets/0263/0263_sam1_13160Hz.wav";
constexpr auto NO_WAVE = "assets/0263/0263_sam2_6453Hz.wav";
constexpr auto MISSING_SAMPLE = "assets/0263/0263_sam3_6453Hz.wav";

void putWord(std::vector<uint8_t> &data, std::size_t at, int value) {
  data[at] = static_cast<uint8_t>(value & 0xFF);
  data[at + 1] = static_cast<uint8_t>(value >> 8);
}

void append(std::vector<uint8_t> &data, uint32_t value, int size) {
  for (int i = 0; i < size; ++i) {
    data.push_back(static_cast<uint8_t>(value >> (8 * i)));
  }
}

void putId(std::vector<uint8_t> &data, const char *id) {
  data.insert(data.end(), id, id + 4);
}

std::vector<uint8_t> toneModule() {
  std::vector<uint8_t> data(0xC0, 0);
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
  data[0x32] = 128;
  data[0x33] = 0xB0;
  for (std::size_t channel = 0; channel < 32; ++channel) {
    data[0x40 + channel] = static_cast<uint8_t>(channel < 4 ? channel : 0xFF);
  }
  data[0x60] = 0;
  data[0x61] = 0xFF;
  putWord(data, 0x62, 0x70 / 16);
  putWord(data, 0x64, 0xC0 / 16);
  data[0x70] = 1;
  putWord(data, 0x70 + 0x0E, 0x110 / 16);
  data[0x70 + 0x10] = 16;
  data[0x70 + 0x18] = 16;
  data[0x70 + 0x1C] = 64;
  data[0x70 + 0x1F] = 1;
  putWord(data, 0x70 + 0x20, 8363);
  data[0x70 + 0x4C] = 'S';
  data[0x70 + 0x4D] = 'C';
  data[0x70 + 0x4E] = 'R';
  data[0x70 + 0x4F] = 'S';
  const std::vector<uint8_t> rowZero = {0x20, 0x40, 1, 0};
  std::vector<uint8_t> pattern(2, 0);
  pattern.insert(pattern.end(), rowZero.begin(), rowZero.end());
  pattern.insert(pattern.end(), 63, 0);
  putWord(pattern, 0, static_cast<int>(pattern.size()));
  data.insert(data.end(), pattern.begin(), pattern.end());
  data.resize(0x110);
  for (int frame = 0; frame < 16; ++frame) {
    data.push_back(static_cast<uint8_t>(frame % 8 < 4 ? 228 : 28));
  }
  return data;
}

std::vector<uint8_t> wave() {
  const std::vector<uint8_t> frames = {128, 255, 0, 128};
  std::vector<uint8_t> file;
  putId(file, "RIFF");
  append(file, 0, 4);
  putId(file, "WAVE");
  putId(file, "fmt ");
  append(file, 16, 4);
  append(file, 1, 2);
  append(file, 1, 2);
  append(file, 13160, 4);
  append(file, 13160, 4);
  append(file, 1, 2);
  append(file, 8, 2);
  putId(file, "data");
  append(file, static_cast<uint32_t>(frames.size()), 4);
  file.insert(file.end(), frames.begin(), frames.end());
  return file;
}

struct Disk {
  std::map<std::string, std::vector<uint8_t>> files = {
      {TUNE, toneModule()},
      {NOISE, std::vector<uint8_t>(256, 0x55)},
      {SAMPLE, wave()},
      {NO_WAVE, std::vector<uint8_t>(64, 0x55)}};
  std::vector<std::string> reads;

  AudioSystem::Read reader() {
    return [this](const std::string &path) {
      reads.push_back(path);
      const auto file = files.find(path);
      if (file == files.end()) {
        throw std::runtime_error("Failed to open " + path);
      }
      return file->second;
    };
  }
};

struct Audio {
  HeadlessSdl sdl;
  Disk disk;
  AudioSystem system{disk.reader()};
};

} // namespace

SCENARIO("Music is read through the files and named by its path") {
  GIVEN("An audio system on files holding a tune") {
    Audio audio;

    THEN("No music is loaded yet") {
      REQUIRE(audio.system.loadedMusic().empty());
    }

    WHEN("The tune is loaded") {
      audio.system.loadMusic(TUNE);

      THEN("It is the loaded music, read once") {
        REQUIRE(audio.system.loadedMusic() == TUNE);
        REQUIRE(audio.disk.reads == std::vector<std::string>{TUNE});
      }

      AND_WHEN("It is played, stopped, played once and updated") {
        audio.system.playMusic();
        audio.system.update();
        audio.system.stopMusic();
        audio.system.playMusicOnce();
        audio.system.update();

        THEN("It stays loaded without being read again") {
          REQUIRE(audio.system.loadedMusic() == TUNE);
          REQUIRE(audio.disk.reads.size() == 1);
        }
      }

      AND_WHEN("It is cleared") {
        audio.system.clearMusic();

        THEN("No music is loaded") {
          REQUIRE(audio.system.loadedMusic().empty());
        }
      }

      AND_WHEN("A missing tune is loaded in its place") {
        REQUIRE_NOTHROW(audio.system.loadMusic(MISSING_TUNE));

        THEN("The old tune is gone and nothing replaces it") {
          REQUIRE(audio.system.loadedMusic().empty());
          REQUIRE(audio.disk.reads ==
                  std::vector<std::string>{TUNE, MISSING_TUNE});
        }
      }

      AND_WHEN("A file that is no module is loaded in its place") {
        audio.system.loadMusic(NOISE);

        THEN("No music is loaded") {
          REQUIRE(audio.system.loadedMusic().empty());
        }
      }
    }

    WHEN("Music is started and stopped with nothing loaded") {
      REQUIRE_NOTHROW(audio.system.playMusic());
      REQUIRE_NOTHROW(audio.system.playMusicOnce());
      REQUIRE_NOTHROW(audio.system.stopMusic());

      THEN("Still no music is loaded and nothing was read") {
        REQUIRE(audio.system.loadedMusic().empty());
        REQUIRE(audio.disk.reads.empty());
      }
    }
  }
}

SCENARIO("Music begun in steps is loaded whole by its single step") {
  GIVEN("An audio system on files holding a tune") {
    Audio audio;

    WHEN("The tune is begun") {
      const std::unique_ptr<Speaker::MusicLoad> load =
          audio.system.beginMusic(TUNE, toneModule(), 4);

      THEN("Nothing is loaded before the step") {
        REQUIRE(audio.system.loadedMusic().empty());
      }

      THEN("The first step finishes with the tune loaded") {
        REQUIRE(load->step());
        REQUIRE(audio.system.loadedMusic() == TUNE);
      }
    }
  }
}

SCENARIO("Music prepared ahead is not kept on the desktop") {
  GIVEN("An audio system with a tune loaded") {
    Audio audio;
    audio.system.loadMusic(TUNE);

    WHEN("Another tune is prepared from its data") {
      audio.system.prepareMusic(NOISE, toneModule());

      THEN("There is nothing to step and the loaded tune stays") {
        REQUIRE_FALSE(audio.system.stepPreparation());
        REQUIRE(audio.system.loadedMusic() == TUNE);
        REQUIRE(audio.disk.reads == std::vector<std::string>{TUNE});
      }

      AND_WHEN("The prepared music is dropped") {
        audio.system.dropPreparedMusic();

        THEN("The loaded tune stays") {
          REQUIRE(audio.system.loadedMusic() == TUNE);
        }
      }
    }
  }
}

SCENARIO("The music volume, tempo and filter leave the tune loaded") {
  GIVEN("An audio system playing a tune") {
    Audio audio;
    audio.system.loadMusic(TUNE);
    audio.system.playMusic();

    WHEN("The music fades out at a doubled tempo through the filter") {
      audio.system.setLowPassFilter(true);
      audio.system.setMusicTempoScale(2.0);
      audio.system.setMusicTempo(14);
      for (int volume = 63; volume >= 0; volume -= 9) {
        audio.system.setMusicVolume(volume);
        audio.system.update();
      }
      audio.system.setLowPassFilter(false);

      THEN("The tune stays loaded and nothing more is read") {
        REQUIRE(audio.system.loadedMusic() == TUNE);
        REQUIRE(audio.disk.reads == std::vector<std::string>{TUNE});
      }
    }
  }
}

SCENARIO("Samples are read once when loaded and played from memory") {
  GIVEN("An audio system on files holding a sample") {
    Audio audio;

    WHEN("The sample is loaded under a name") {
      audio.system.loadSample("hit", SAMPLE);

      THEN("Its file is read") {
        REQUIRE(audio.disk.reads == std::vector<std::string>{SAMPLE});
      }

      AND_WHEN("It is played, looped, stopped and cleared") {
        audio.system.playSample("hit", 0x1);
        audio.system.playSampleAt("hit", 0xF, 8000);
        audio.system.playSampleAt("hit", 0x2, 0);
        audio.system.setSampleLooping(true);
        audio.system.playSample("hit", 0x6);
        audio.system.setSampleLooping(false);
        audio.system.update();
        audio.system.stopSamples();
        audio.system.clearSample("hit");

        THEN("Its file is not read again") {
          REQUIRE(audio.disk.reads.size() == 1);
        }
      }

      AND_WHEN("It is loaded again under the same name") {
        audio.system.loadSample("hit", SAMPLE);

        THEN("The file is read again") {
          REQUIRE(audio.disk.reads == std::vector<std::string>{SAMPLE, SAMPLE});
        }
      }
    }

    WHEN("A missing sample and a file that is no wave are loaded") {
      REQUIRE_NOTHROW(audio.system.loadSample("missing", MISSING_SAMPLE));
      REQUIRE_NOTHROW(audio.system.loadSample("noise", NO_WAVE));

      THEN("Both are tried and playing them is quietly ignored") {
        REQUIRE(audio.disk.reads ==
                std::vector<std::string>{MISSING_SAMPLE, NO_WAVE});
        REQUIRE_NOTHROW(audio.system.playSample("missing", 0x1));
        REQUIRE_NOTHROW(audio.system.playSampleAt("noise", 0x1, 8000));
      }
    }

    WHEN("A name that was never loaded is played and cleared") {
      REQUIRE_NOTHROW(audio.system.playSample("never", 0x1));
      REQUIRE_NOTHROW(audio.system.playSampleAt("never", 0x1, 8000));
      REQUIRE_NOTHROW(audio.system.clearSample("never"));

      THEN("Nothing is read") { REQUIRE(audio.disk.reads.empty()); }
    }
  }
}
