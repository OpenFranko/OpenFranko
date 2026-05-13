#include "../../../lib/converter/abkToS3m/abkToS3m.h"
#include "../../../lib/decompressor/helpers/helpers.h"
#include <catch2/catch_all.hpp>
#include <cstring>
#include <vector>

using namespace openfranko::lib::converter::abkToS3m;
using namespace openfranko::lib::decompressor::helpers;

namespace {

std::vector<uint8_t> buildMinimalAbk(const char *songName = "test song",
                                     uint16_t amosTempo = 17) {
  std::vector<uint8_t> music;

  size_t sampleInfoOff = 12;
  size_t songOff = 12 + 36;
  size_t trackOff = songOff + 6 + 28 + 6;

  pushBigEndian32(music, static_cast<uint32_t>(sampleInfoOff));
  pushBigEndian32(music, static_cast<uint32_t>(songOff));
  pushBigEndian32(music, static_cast<uint32_t>(trackOff));

  pushBigEndian16(music, 1);
  pushBigEndian32(music, 34);
  pushBigEndian32(music, 34);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 1);
  pushBigEndian16(music, 63);
  pushBigEndian16(music, 1);
  char sampleName[16] = "testsample";
  music.insert(music.end(), sampleName, sampleName + 16);
  music.push_back(0x40);
  music.push_back(0xC0);

  pushBigEndian16(music, 0);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 6);
  pushBigEndian16(music, 20);
  pushBigEndian16(music, 22);
  pushBigEndian16(music, 24);
  pushBigEndian16(music, 26);
  pushBigEndian16(music, amosTempo);
  pushBigEndian16(music, 0);
  char nameBuf[16] = {};
  std::strncpy(nameBuf, songName, 15);
  music.insert(music.end(), nameBuf, nameBuf + 16);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 0xFFFF);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 0xFFFF);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 0xFFFF);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 0xFFFF);

  size_t expectedTrackOff = trackOff;
  while (music.size() < expectedTrackOff) {
    music.push_back(0);
  }

  pushBigEndian16(music, 1);
  uint16_t patternDataOff = static_cast<uint16_t>(2 + 4 * 2);
  pushBigEndian16(music, patternDataOff);
  pushBigEndian16(music, patternDataOff);
  pushBigEndian16(music, patternDataOff);
  pushBigEndian16(music, patternDataOff);
  music.push_back(0x80);
  music.push_back(0x00);

  std::vector<uint8_t> abk;
  abk.push_back('A');
  abk.push_back('m');
  abk.push_back('B');
  abk.push_back('k');
  pushBigEndian16(abk, 3);
  pushBigEndian16(abk, 0);
  pushBigEndian32(abk, static_cast<uint32_t>(music.size() + 8) | 0x80000000u);
  const char *bankName = "Music   ";
  abk.insert(abk.end(), bankName, bankName + 8);
  abk.insert(abk.end(), music.begin(), music.end());

  return abk;
}

} // namespace

SCENARIO("convert rejects invalid input") {
  GIVEN("A buffer that is too small") {
    std::vector<uint8_t> data = {0x01, 0x02};

    THEN("it throws") { REQUIRE_THROWS_AS(convert(data), std::runtime_error); }
  }

  GIVEN("A buffer without AmBk magic") {
    std::vector<uint8_t> data(30, 0);
    data[0] = 'X';

    THEN("it throws") { REQUIRE_THROWS_AS(convert(data), std::runtime_error); }
  }
}

SCENARIO("convert produces a valid S3M file") {
  GIVEN("A minimal valid ABK file") {
    auto abk = buildMinimalAbk();

    WHEN("convert is called") {
      auto s3m = convert(abk);

      THEN("output has S3M signature at offset 0x2C") {
        REQUIRE(s3m.size() > 0x30);
        REQUIRE(s3m[0x2C] == 'S');
        REQUIRE(s3m[0x2D] == 'C');
        REQUIRE(s3m[0x2E] == 'R');
        REQUIRE(s3m[0x2F] == 'M');
      }

      THEN("output has EOF marker at 0x1C") { REQUIRE(s3m[0x1C] == 0x1A); }

      THEN("song name is embedded in the header") {
        std::string name(reinterpret_cast<const char *>(s3m.data()), 9);
        REQUIRE(name == "test song");
      }

      THEN("order count is at least 2 (padded to even)") {
        uint16_t ordNum = readUint16LittleEndian(s3m, 0x20);
        REQUIRE(ordNum >= 2);
        REQUIRE(ordNum % 2 == 0);
      }

      THEN("instrument count is 1") {
        REQUIRE(readUint16LittleEndian(s3m, 0x22) == 1);
      }

      THEN("pattern count is at least 1") {
        REQUIRE(readUint16LittleEndian(s3m, 0x24) >= 1);
      }

      THEN("global volume is 64") { REQUIRE(s3m[0x30] == 64); }
    }
  }
}

SCENARIO("convert handles isE1 special case") {
  GIVEN("An ABK with song name 'e1'") {
    auto abk = buildMinimalAbk("e1", 33);

    WHEN("convert is called") {
      auto s3m = convert(abk);

      THEN("speed is hardcoded to 3") { REQUIRE(s3m[0x31] == 3); }

      THEN("tempo is hardcoded to 130") { REQUIRE(s3m[0x32] == 130); }
    }
  }
}

SCENARIO("convert maps AMOS tempo to S3M speed/tempo") {
  GIVEN("An ABK with AMOS tempo 25") {
    auto abk = buildMinimalAbk("level1", 25);

    WHEN("convert is called") {
      auto s3m = convert(abk);

      THEN("speed = round(100/25) = 4") { REQUIRE(s3m[0x31] == 4); }

      THEN("BPM = round(125*4*25/95) = 132") {
        uint8_t bpm = s3m[0x32];
        REQUIRE(bpm >= 130);
        REQUIRE(bpm <= 135);
      }
    }
  }

  GIVEN("An ABK with AMOS tempo 17 (default)") {
    auto abk = buildMinimalAbk("default", 17);

    WHEN("convert is called") {
      auto s3m = convert(abk);

      THEN("speed = round(100/17) = 6") { REQUIRE(s3m[0x31] == 6); }
    }
  }
}
