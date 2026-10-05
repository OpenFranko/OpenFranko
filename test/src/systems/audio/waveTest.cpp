#include "../../../../src/systems/audio/Wave.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace openfranko::src::systems::audio;

namespace {

void put(std::vector<uint8_t> &file, uint32_t value, int size) {
  for (int i = 0; i < size; ++i) {
    file.push_back(static_cast<uint8_t>(value >> (8 * i)));
  }
}

void putId(std::vector<uint8_t> &file, const char *id) {
  file.insert(file.end(), id, id + 4);
}

struct WaveFile {
  uint16_t tag = 1;
  uint16_t channels = 1;
  uint32_t rate = 6453;
  uint16_t bits = 8;
  std::vector<uint8_t> data = {128, 255, 0};
  std::vector<uint8_t> extra;
  bool withData = true;

  std::vector<uint8_t> bytes() const {
    std::vector<uint8_t> file;
    putId(file, "RIFF");
    put(file, 0, 4);
    putId(file, "WAVE");
    putId(file, "fmt ");
    put(file, 16, 4);
    put(file, tag, 2);
    put(file, channels, 2);
    put(file, rate, 4);
    put(file, rate * channels * bits / 8, 4);
    put(file, channels * bits / 8, 2);
    put(file, bits, 2);
    if (!extra.empty()) {
      putId(file, "LIST");
      put(file, static_cast<uint32_t>(extra.size()), 4);
      file.insert(file.end(), extra.begin(), extra.end());
      if (extra.size() % 2 != 0) {
        file.push_back(0);
      }
    }
    if (withData) {
      putId(file, "data");
      put(file, static_cast<uint32_t>(data.size()), 4);
      file.insert(file.end(), data.begin(), data.end());
    }
    return file;
  }
};

} // namespace

SCENARIO("readWave reads the sample files the asset pipeline writes") {
  GIVEN("An 8-bit mono WAVE file") {
    const WaveFile file;

    WHEN("It is read") {
      const Sound sound = readWave(file.bytes());

      THEN("It keeps its rate") { REQUIRE(sound.rate == 6453); }

      THEN("Its unsigned bytes become signed 8-bit frames") {
        REQUIRE(sound.frames == std::vector<int8_t>{0, 127, -128});
      }
    }
  }

  GIVEN("A 16-bit stereo WAVE file") {
    WaveFile file;
    file.channels = 2;
    file.bits = 16;
    file.data = {0x00, 0x10, 0x00, 0x30, 0x00, 0x80, 0x00, 0x80};

    THEN("Each frame is the high byte of the mean of its two channels") {
      REQUIRE(readWave(file.bytes()).frames == std::vector<int8_t>{0x20, -128});
    }
  }

  GIVEN("A chunk of odd size before the data") {
    WaveFile file;
    file.extra = {1, 2, 3};

    THEN("It is skipped with its pad byte") {
      REQUIRE(readWave(file.bytes()).frames.size() == 3);
    }
  }

  GIVEN("Files the game never uses") {
    THEN("Compressed samples are refused") {
      WaveFile file;
      file.tag = 2;
      REQUIRE_THROWS_AS(readWave(file.bytes()), std::runtime_error);
    }

    THEN("A file without sample data is refused") {
      WaveFile file;
      file.withData = false;
      REQUIRE_THROWS_AS(readWave(file.bytes()), std::runtime_error);
    }

    THEN("Anything but a RIFF WAVE file is refused") {
      std::vector<uint8_t> file = WaveFile().bytes();
      file[0] = 'X';
      REQUIRE_THROWS_AS(readWave(file), std::runtime_error);
    }
  }
}

namespace {

constexpr int8_t FLIPPED = 9;

bool flipToConstant(const uint8_t *, int8_t *target, std::size_t count) {
  std::fill(target, target + count, FLIPPED);
  return true;
}

bool refuseFlip(const uint8_t *, int8_t *, std::size_t) { return false; }

} // namespace

SCENARIO("readWave mixes 8-bit stereo down to mono") {
  GIVEN("An 8-bit stereo WAVE file") {
    WaveFile file;
    file.channels = 2;
    file.data = {144, 176, 0, 255, 128, 128};

    THEN("Each frame is the mean of its two channels") {
      REQUIRE(readWave(file.bytes()).frames == std::vector<int8_t>{32, -1, 0});
    }
  }
}

SCENARIO("readWave lets a sign flip convert 8-bit mono samples") {
  GIVEN("An 8-bit mono WAVE file") {
    const WaveFile file;

    THEN("A flip that does the work gives its frames") {
      REQUIRE(readWave(file.bytes(), flipToConstant).frames ==
              std::vector<int8_t>(3, FLIPPED));
    }

    THEN("A flip that refuses leaves the work to the reader") {
      REQUIRE(readWave(file.bytes(), refuseFlip).frames ==
              std::vector<int8_t>{0, 127, -128});
    }
  }

  GIVEN("A 16-bit mono WAVE file") {
    WaveFile file;
    file.bits = 16;
    file.data = {0x00, 0x10, 0x00, 0xF0};

    THEN("The flip is not asked") {
      REQUIRE(readWave(file.bytes(), flipToConstant).frames ==
              std::vector<int8_t>{0x10, -0x10});
    }
  }
}

SCENARIO("readWave refuses a format chunk cut short") {
  GIVEN("A file whose format chunk has 12 of its 16 bytes") {
    std::vector<uint8_t> file = WaveFile().bytes();
    file[16] = 12;
    file.resize(12 + 8 + 12);

    THEN("It is refused") {
      REQUIRE_THROWS_AS(readWave(file), std::runtime_error);
    }
  }
}
