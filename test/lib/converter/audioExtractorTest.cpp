#include "../../../lib/converter/audioExtractor/audioExtractor.h"
#include "../../../lib/decompressor/helpers/helpers.h"
#include <catch2/catch_all.hpp>
#include <cstring>
#include <vector>

using namespace openfranko::lib::converter::audioExtractor;
using namespace openfranko::lib::decompressor::helpers;

namespace {

std::vector<uint8_t> buildStandaloneSamBank(uint16_t freq, uint32_t pcmLen,
                                            const std::vector<int8_t> &pcm) {
  std::vector<uint8_t> data;
  pushBigEndian16(data, 1);
  uint32_t sampleOffset = 6;
  pushBigEndian32(data, sampleOffset);

  pushBigEndian32(data, 0);
  pushBigEndian32(data, 0);
  pushBigEndian16(data, freq);
  pushBigEndian32(data, pcmLen);
  for (auto s : pcm) {
    data.push_back(static_cast<uint8_t>(s));
  }
  return data;
}

} // namespace

SCENARIO("extractStandaloneSamBank extracts WAV from sample bank") {
  GIVEN("A sample bank with one sample") {
    std::vector<int8_t> pcm = {0, 10, -10, 50, -50};
    auto data = buildStandaloneSamBank(8000, 5, pcm);

    WHEN("extractStandaloneSamBank is called") {
      auto results = extractStandaloneSamBank(data, "TEST");

      THEN("it extracts one sample") { REQUIRE(results.size() == 1); }

      THEN("the name includes file ID and frequency") {
        REQUIRE(results[0].name == "TEST_sam1_8000Hz.wav");
      }

      THEN("the output is a valid WAV file") {
        auto &wav = results[0].data;
        REQUIRE(wav.size() >= 44);
        REQUIRE(wav[0] == 'R');
        REQUIRE(wav[1] == 'I');
        REQUIRE(wav[2] == 'F');
        REQUIRE(wav[3] == 'F');
        REQUIRE(wav[8] == 'W');
        REQUIRE(wav[9] == 'A');
        REQUIRE(wav[10] == 'V');
        REQUIRE(wav[11] == 'E');
      }

      THEN("WAV sample rate matches input frequency") {
        auto &wav = results[0].data;
        REQUIRE(readUint32LittleEndian(wav, 24) == 8000);
      }

      THEN("WAV has correct number of samples") {
        auto &wav = results[0].data;
        uint32_t dataLen = readUint32LittleEndian(wav, 40);
        REQUIRE(dataLen == 10);
      }
    }
  }

  GIVEN("A buffer that is too small") {
    std::vector<uint8_t> data = {0x01, 0x02};

    WHEN("extractStandaloneSamBank is called") {
      auto results = extractStandaloneSamBank(data, "X");
      THEN("it returns empty") { REQUIRE(results.empty()); }
    }
  }

  GIVEN("A sample bank with zero samples") {
    std::vector<uint8_t> data;
    pushBigEndian16(data, 0);
    pushBigEndian32(data, 0);

    WHEN("extractStandaloneSamBank is called") {
      auto results = extractStandaloneSamBank(data, "X");
      THEN("it returns empty") { REQUIRE(results.empty()); }
    }
  }

  GIVEN("A sample with zero frequency") {
    std::vector<int8_t> pcm = {42};
    auto data = buildStandaloneSamBank(0, 1, pcm);

    WHEN("extractStandaloneSamBank is called") {
      auto results = extractStandaloneSamBank(data, "F0");
      THEN("frequency defaults to 8287") {
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].name == "F0_sam1_8287Hz.wav");
        auto &wav = results[0].data;
        REQUIRE(readUint32LittleEndian(wav, 24) == 8287);
      }
    }
  }
}

SCENARIO("wrapMusicBank produces a valid ABK wrapper") {
  GIVEN("Minimal music data (12+ bytes with 3 offset pointers)") {
    std::vector<uint8_t> music(20, 0);

    WHEN("wrapMusicBank is called") {
      auto result = wrapMusicBank(music, "025F");

      THEN("the name has .abk extension") {
        REQUIRE(result.name == "025F.abk");
      }

      THEN("output starts with AmBk magic") {
        REQUIRE(result.data.size() >= 20);
        REQUIRE(result.data[0] == 'A');
        REQUIRE(result.data[1] == 'm');
        REQUIRE(result.data[2] == 'B');
        REQUIRE(result.data[3] == 'k');
      }

      THEN("music data follows the 20-byte header") {
        REQUIRE(result.data.size() == 20 + music.size());
      }
    }
  }
}

SCENARIO("extractEmbeddedSamBank handles edge cases") {
  GIVEN("A buffer smaller than 12 bytes") {
    std::vector<uint8_t> data = {0, 1, 2, 3};

    WHEN("extractEmbeddedSamBank is called") {
      auto results = extractEmbeddedSamBank(data, "X");
      THEN("it returns empty") { REQUIRE(results.empty()); }
    }
  }

  GIVEN("A buffer with sbOff pointing out of range") {
    std::vector<uint8_t> data(20, 0);
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0xFF;
    data[11] = 0xFF;

    WHEN("extractEmbeddedSamBank is called") {
      auto results = extractEmbeddedSamBank(data, "X");
      THEN("it returns empty") { REQUIRE(results.empty()); }
    }
  }

  GIVEN("A buffer with sbOff = 0 (no sample bank)") {
    std::vector<uint8_t> data(20, 0);

    WHEN("extractEmbeddedSamBank is called") {
      auto results = extractEmbeddedSamBank(data, "X");
      THEN("it returns empty") { REQUIRE(results.empty()); }
    }
  }
}
