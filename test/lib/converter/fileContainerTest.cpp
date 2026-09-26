#include "../../../lib/converter/fileContainer/fileContainer.h"
#include "../../../lib/converter/gameData/gameData.h"
#include "../../../lib/helpers/helpers.h"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::lib::converter::fileContainer;
namespace gameData = openfranko::lib::converter::gameData;
namespace resourceTypes = openfranko::lib::converter::gameData::resourceTypes;
using openfranko::lib::helpers::BigEndianReader;
using openfranko::lib::helpers::pushBigEndian16;
using openfranko::lib::helpers::pushBigEndian32;

namespace {

std::vector<uint8_t> squash(const std::vector<uint8_t> &data) {
  std::vector<bool> bits;
  const auto put = [&bits](uint32_t value, int count) {
    for (int i = count - 1; i >= 0; --i) {
      bits.push_back(((value >> i) & 1u) != 0);
    }
  };
  for (size_t end = data.size(); end > 0;) {
    const size_t run = std::min<size_t>(8, end);
    put(0, 2);
    put(static_cast<uint32_t>(run - 1), 3);
    for (size_t i = 0; i < run; ++i) {
      put(data[--end], 8);
    }
  }
  std::vector<uint32_t> words((bits.size() + 31) / 32, 0);
  for (size_t i = 0; i < bits.size(); ++i) {
    words[i / 32] |= static_cast<uint32_t>(bits[i]) << (i % 32);
  }
  std::vector<uint8_t> stream;
  uint32_t checksum = 1;
  for (auto word = words.rbegin(); word != words.rend(); ++word) {
    pushBigEndian32(stream, *word);
    checksum ^= *word;
  }
  pushBigEndian32(stream, 1);
  pushBigEndian32(stream, checksum);
  pushBigEndian32(stream, static_cast<uint32_t>(data.size()));
  return stream;
}

std::vector<uint8_t> withLongLength(const std::vector<uint8_t> &stream) {
  std::vector<uint8_t> file;
  pushBigEndian32(file, static_cast<uint32_t>(stream.size()));
  file.insert(file.end(), stream.begin(), stream.end());
  return file;
}

std::vector<uint8_t> withWordLength(const std::vector<uint8_t> &stream) {
  std::vector<uint8_t> file;
  pushBigEndian16(file, static_cast<uint16_t>(stream.size()));
  file.insert(file.end(), stream.begin(), stream.end());
  return file;
}

std::vector<uint8_t> pictureHeader(uint16_t planes) {
  std::vector<uint8_t> picture;
  pushBigEndian32(picture, 0x06071963);
  pushBigEndian32(picture, 0);
  pushBigEndian16(picture, 1);
  pushBigEndian16(picture, 1);
  pushBigEndian16(picture, 1);
  pushBigEndian16(picture, planes);
  return picture;
}

void append(std::vector<uint8_t> &data, const std::vector<uint8_t> &more) {
  data.insert(data.end(), more.begin(), more.end());
}

} // namespace

SCENARIO("parseFooter reads the last 8 bytes as big-endian fields") {
  GIVEN("A 16-byte buffer with a known 8-byte suffix") {
    std::vector<uint8_t> data = {
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,
        0x00, 0x01, 0x00, 0x00,
        0x03, 0x84,
        0x02, 0x00,
    };

    WHEN("parseFooter is called") {
      auto info = parseFooter(data);

      THEN("unpackSize is read correctly") { REQUIRE(info.unpackSize == 65536); }

      THEN("fileId is read correctly") { REQUIRE(info.fileId == 0x0384); }

      THEN("resourceType is read correctly") {
        REQUIRE(info.resourceType == 0x0200);
      }
    }
  }

  GIVEN("An exactly 8-byte buffer") {
    std::vector<uint8_t> data = {
        0x00, 0x00, 0x00, 0x42,
        0x00, 0x38,
        0x00, 0x00,
    };

    WHEN("parseFooter is called") {
      auto info = parseFooter(data);

      THEN("All fields are parsed from the entire buffer") {
        REQUIRE(info.unpackSize == 66);
        REQUIRE(info.fileId == 0x0038);
        REQUIRE(info.resourceType == 0x0000);
      }
    }
  }

  GIVEN("A buffer smaller than 8 bytes") {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};

    WHEN("parseFooter is called") {
      THEN("It throws a runtime_error") {
        REQUIRE_THROWS_AS(parseFooter(data), std::runtime_error);
      }
    }
  }
}

SCENARIO("fileIdToHex formats a file ID like the game's file names") {
  GIVEN("File IDs across the 16-bit range") {
    THEN("They become four upper-case hex digits") {
      REQUIRE(fileIdToHex(0x0385) == "0385");
      REQUIRE(fileIdToHex(0x03BE) == "03BE");
      REQUIRE(fileIdToHex(0x0000) == "0000");
      REQUIRE(fileIdToHex(0xFFFF) == "FFFF");
    }
  }
}

SCENARIO("unpack reads a version 1.0 file through its footer") {
  GIVEN("A squashed file whose footer names it 0385, an icon bank") {
    auto file = squash({1, 2, 3});
    pushBigEndian32(file, 3);
    pushBigEndian16(file, 0x0385);
    pushBigEndian16(file, 0x0200);

    WHEN("It is unpacked") {
      const auto resource = unpack("anything", file);

      THEN("The footer names it and the data is unsquashed") {
        REQUIRE(resource.fileId == "0385");
        REQUIRE(resource.resourceType == resourceTypes::ICONS);
        REQUIRE(resource.data == std::vector<uint8_t>{1, 2, 3});
      }
    }
  }

  GIVEN("A file whose footer says it is stored unsquashed") {
    std::vector<uint8_t> file = {0x12, 0x03, 0x19, 0x90};
    pushBigEndian32(file, 4);
    pushBigEndian16(file, 0x03B8);
    pushBigEndian16(file, 0x0201);

    WHEN("It is unpacked") {
      const auto resource = unpack("03B8", file);

      THEN("The data is the file without its footer") {
        REQUIRE(resource.fileId == "03B8");
        REQUIRE(resource.resourceType == resourceTypes::SCREEN_PACKAGE);
        REQUIRE(resource.data == std::vector<uint8_t>{0x12, 0x03, 0x19, 0x90});
      }
    }
  }
}

SCENARIO("unpack reads a version 1.2 file the way its loader procedure does") {
  GIVEN("p58, loaded by _DATA: a 32-bit packed length, then the block") {
    const auto file =
        withLongLength(squash({0, 0, 0, 3, 58, 0, 7, 8, 9, 0xEE}));

    WHEN("It is unpacked") {
      const auto resource = unpack("p58", file);

      THEN("The bank is as long as the block says, after its 6-byte header") {
        REQUIRE(resource.fileId == "p58");
        REQUIRE(resource.resourceType == resourceTypes::ICONS);
        REQUIRE(resource.data == std::vector<uint8_t>{7, 8, 9});
      }
    }
  }

  GIVEN("p1, loaded by _DATA16: a 16-bit packed length, then the block") {
    const auto file =
        withWordLength(squash({0, 4, 1, 0, 0xA1, 0xA2, 0xA3, 0xA4}));

    WHEN("It is unpacked") {
      const auto resource = unpack("p1", file);

      THEN("The loader copies two bytes too few, so the bank ends in zeros") {
        REQUIRE(resource.fileId == "p1");
        REQUIRE(resource.resourceType == resourceTypes::ICONS);
        REQUIRE(resource.data == std::vector<uint8_t>{0xA1, 0xA2, 0, 0});
      }
    }
  }

  GIVEN("m1, loaded by _MUSIC") {
    const auto file = withLongLength(squash({0, 0, 0, 2, 1, 0, 0x4D, 0x42}));

    WHEN("It is unpacked") {
      const auto resource = unpack("m1", file);

      THEN("It is a music bank with the block's payload") {
        REQUIRE(resource.resourceType == resourceTypes::MUSIC);
        REQUIRE(resource.data == std::vector<uint8_t>{0x4D, 0x42});
      }
    }
  }

  GIVEN("t11, loaded by _STAGE into bank 10 two bytes past the length") {
    const auto file = withWordLength(squash({0x0B, 0x3F, 0x00, 0x08}));

    WHEN("It is unpacked") {
      const auto resource = unpack("t11", file);

      THEN("The bank keeps the packed length in front of the block") {
        const std::vector<uint8_t> expected = {file[0], file[1], 0x0B,
                                               0x3F,    0x00,    0x08};
        REQUIRE(resource.resourceType == resourceTypes::ICONS);
        REQUIRE(resource.data == expected);
      }
    }
  }

  GIVEN("p52, whose longwords are stored rotated right by 5 bits") {
    std::vector<uint8_t> file;
    pushBigEndian32(file, 0x12031990u >> 5 | 0x12031990u << 27);
    pushBigEndian32(file, 0x80000001u >> 5 | 0x80000001u << 27);

    WHEN("It is unpacked") {
      const auto resource = unpack("p52", file);

      THEN("Rol.l 5 restores a screen package") {
        REQUIRE(resource.resourceType == resourceTypes::SCREEN_PACKAGE);
        REQUIRE(resource.data == std::vector<uint8_t>{0x12, 0x03, 0x19, 0x90,
                                                      0x80, 0x00, 0x00, 0x01});
      }
    }
  }

  GIVEN("A file shorter than the packed length in front of it") {
    auto file = withLongLength(squash({0, 0, 0, 1, 0, 0, 5}));
    file.pop_back();

    WHEN("It is unpacked") {
      THEN("It throws a runtime_error") {
        REQUIRE_THROWS_AS(unpack("p0", file), std::runtime_error);
      }
    }
  }
}

SCENARIO("unpack gives a version 1.2 bob bank the version 1.0 layout") {
  std::vector<uint8_t> table;
  for (uint16_t value : {10, 1, 2, 3, 4, 18, 2, 5, 6, 7}) {
    pushBigEndian16(table, value);
  }
  std::vector<uint8_t> pictures = pictureHeader(4);
  append(pictures, pictureHeader(5));

  GIVEN("s1 with two bobs on a full-width screen and a 4-byte sample bank") {
    std::vector<uint8_t> block;
    pushBigEndian32(block, 66);
    block.push_back(2);
    block.push_back(1);
    pushBigEndian16(block, 4);
    block.push_back(0);
    block.push_back(40);
    append(block, table);
    append(block, pictures);
    append(block, {0, 1, 0xAA, 0xBB, 0, 0, 0, 0});

    WHEN("It is unpacked") {
      const auto resource = unpack("s1", withLongLength(squash(block)));
      BigEndianReader reader(resource.data);

      THEN("The header gives the count, a 320-pixel width, the height, the "
           "colours of the deepest picture and where the samples start") {
        REQUIRE(resource.resourceType == resourceTypes::SPRITES);
        REQUIRE(reader.readUint16(0) == 2);
        REQUIRE(reader.readUint16(2) == 320);
        REQUIRE(reader.readUint16(4) == 40);
        REQUIRE(reader.readUint16(6) == 32);
        REQUIRE(reader.readUint32(8) == 64);
      }

      THEN("The descriptors and pictures follow unchanged, then the samples") {
        std::vector<uint8_t> expected = table;
        append(expected, pictures);
        append(expected, {0, 1, 0xAA, 0xBB});
        REQUIRE(std::vector<uint8_t>(resource.data.begin() + 12,
                                     resource.data.end()) == expected);
      }
    }
  }

  GIVEN("s0 with two bobs on a 96-pixel screen and no samples") {
    std::vector<uint8_t> block;
    pushBigEndian32(block, 66);
    block.push_back(2);
    block.push_back(0);
    pushBigEndian16(block, 0);
    block.push_back(96);
    block.push_back(23);
    append(block, table);
    append(block, pictures);
    append(block, {0, 0, 0, 0});

    WHEN("It is unpacked") {
      const auto resource = unpack("s0", withLongLength(squash(block)));
      BigEndianReader reader(resource.data);

      THEN("There is no sample bank and the pictures end the data") {
        REQUIRE(reader.readUint16(2) == 96);
        REQUIRE(reader.readUint16(4) == 23);
        REQUIRE(reader.readUint32(8) == 0);
        REQUIRE(resource.data.size() == 12 + table.size() + pictures.size());
      }
    }
  }
}

SCENARIO("unsquashVersion12 gives the whole squashed block of a 1.2 file") {
  GIVEN("t11 and its block") {
    const std::vector<uint8_t> block = {0x0B, 0x3F, 0x00, 0x08};

    THEN("The block comes back with its own header") {
      REQUIRE(unsquashVersion12("t11", withWordLength(squash(block))) == block);
    }
  }

  GIVEN("A file that is not squashed and a 1.0 file name") {
    THEN("It throws a runtime_error") {
      REQUIRE_THROWS_AS(unsquashVersion12("p52", {0, 0, 0, 0}),
                        std::runtime_error);
      REQUIRE_THROWS_AS(unsquashVersion12("0384", squash({1})),
                        std::runtime_error);
    }
  }
}

SCENARIO("version10Id names the 1.0 file whose role a 1.2 file has") {
  GIVEN("1.2 files with and without a 1.0 counterpart, and a 1.0 file") {
    THEN("Each maps to its counterpart, new files to nothing") {
      REQUIRE(gameData::version10Id("s56") == "0038");
      REQUIRE(gameData::version10Id("t40") == "0154");
      REQUIRE(gameData::version10Id("p52") == "03B8");
      REQUIRE(gameData::version10Id("s50").empty());
      REQUIRE(gameData::version10Id("0384") == "0384");
    }
  }
}
