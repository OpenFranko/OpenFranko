#include "../../../../src/engine/assets/PackedFiles.h"
#include "../../../../lib/converter/packedArchive/lz4Compressor.h"
#include "../../../../lib/converter/packedArchive/packedArchive.h"
#include "../../../../src/engine/assets/Assets.h"
#include "../../../../src/engine/assets/Lz4.h"
#include "../../../../src/engine/assets/PackedArchive.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace openfranko;
namespace packedArchive = openfranko::lib::converter::packedArchive;
using namespace openfranko::src::engine::assets;
using openfranko::src::systems::graphics::IndexedBitmap;

namespace {

std::vector<uint8_t> bytes(const std::string &text) {
  return std::vector<uint8_t>(text.begin(), text.end());
}

std::vector<uint8_t> pattern(std::size_t size, uint32_t seed) {
  std::vector<uint8_t> data(size);
  uint32_t state = seed;
  for (std::size_t at = 0; at < size; ++at) {
    state = state * 1103515245u + 12345u;
    data[at] = at % 97 < 60 ? static_cast<uint8_t>(at % 7)
                            : static_cast<uint8_t>(state >> 24);
  }
  return data;
}

IndexedBitmap bitmap() {
  IndexedBitmap picture;
  picture.width = 37;
  picture.height = 5;
  picture.hotspotX = -3;
  picture.hotspotY = 4;
  picture.palette = {0x000, 0xF00, 0x0F0, 0x00F};
  for (int at = 0; at < picture.width * picture.height; ++at) {
    picture.pixels.push_back(static_cast<uint8_t>(at % 4));
  }
  return picture;
}

std::vector<uint8_t> archive() {
  packedArchive::ArchiveWriter writer;
  writer.addFile("assets/music/song.s3m", pattern(5000, 1));
  writer.addFile("assets/text.txt", bytes("hello"));
  writer.addFile("assets/music/empty.bin", {});
  writer.addBitmap("assets/0384/0384.bmp", bitmap());
  return writer.finish();
}

} // namespace

SCENARIO("LZ4 blocks decompress to what was compressed") {
  GIVEN("Data with long repeats and with noise") {
    for (const std::size_t size : {0u, 1u, 15u, 300u, 70000u}) {
      const std::vector<uint8_t> data =
          pattern(size, static_cast<uint32_t>(size));
      const std::vector<uint8_t> packed =
          packedArchive::compressLz4(data.data(), data.size());
      std::vector<uint8_t> unpacked(size);
      decompressLz4(packed.data(), packed.size(), unpacked.data(),
                    unpacked.size());
      REQUIRE(unpacked == data);
    }
  }

  GIVEN("Data with long runs of one byte, like transparent sprite rows") {
    std::vector<uint8_t> data(5000, 0);
    for (std::size_t at = 0; at < data.size(); at += 97) {
      data[at] = static_cast<uint8_t>(at);
      data[at / 2] = 7;
    }
    const std::vector<uint8_t> packed =
        packedArchive::compressLz4(data.data(), data.size());
    std::vector<uint8_t> unpacked(data.size(), 0xAA);
    decompressLz4(packed.data(), packed.size(), unpacked.data(),
                  unpacked.size());
    REQUIRE(unpacked == data);
  }

  GIVEN("A truncated block") {
    const std::vector<uint8_t> data = pattern(1000, 3);
    std::vector<uint8_t> packed =
        packedArchive::compressLz4(data.data(), data.size());
    packed.resize(packed.size() / 2);
    std::vector<uint8_t> unpacked(data.size());
    THEN("Decompressing it fails instead of reading past the end") {
      REQUIRE_THROWS_AS(decompressLz4(packed.data(), packed.size(),
                                      unpacked.data(), unpacked.size()),
                        std::runtime_error);
    }
  }
}

SCENARIO("LZ4 blocks decompress in steps of a few bytes") {
  GIVEN("Data with repeats, noise and runs") {
    const std::vector<uint8_t> data = pattern(70000, 9);
    const std::vector<uint8_t> packed =
        packedArchive::compressLz4(data.data(), data.size());

    THEN("Any step size gives the same bytes") {
      for (const std::size_t bytes : {1u, 7u, 256u, 4096u, 100000u}) {
        CAPTURE(bytes);
        std::vector<uint8_t> unpacked(data.size(), 0xAA);
        Lz4Steps steps(packed.data(), packed.size(), unpacked.data(),
                       unpacked.size());
        std::size_t taken = 1;
        while (!steps.step(bytes)) {
          ++taken;
        }
        REQUIRE(unpacked == data);
        REQUIRE(taken <= data.size() / bytes + 2);
      }
    }

    THEN("Each step stops at a sequence boundary past its share") {
      std::vector<uint8_t> unpacked(data.size());
      const uint8_t *source = packed.data();
      uint8_t *target = unpacked.data();
      decompressLz4Part(source, packed.data() + packed.size(), unpacked.data(),
                        target, unpacked.data() + unpacked.size(),
                        unpacked.data() + 1000);
      REQUIRE(target >= unpacked.data() + 1000);
      REQUIRE(source < packed.data() + packed.size());
      decompressLz4Part(source, packed.data() + packed.size(), unpacked.data(),
                        target, unpacked.data() + unpacked.size(), nullptr);
      REQUIRE(source == packed.data() + packed.size());
      REQUIRE(unpacked == data);
    }
  }

  GIVEN("An empty block") {
    const std::vector<uint8_t> packed = packedArchive::compressLz4(nullptr, 0);

    THEN("It is done in one step") {
      uint8_t unused = 0;
      Lz4Steps steps(packed.data(), packed.size(), &unused, 0);
      REQUIRE(steps.step(64));
    }
  }

  GIVEN("Blocks that end too soon or run too long") {
    const std::vector<uint8_t> data = pattern(5000, 4);
    std::vector<uint8_t> packed =
        packedArchive::compressLz4(data.data(), data.size());

    THEN("Stepping through them fails") {
      const auto run = [](const std::vector<uint8_t> &block, std::size_t size) {
        std::vector<uint8_t> unpacked(size);
        Lz4Steps steps(block.data(), block.size(), unpacked.data(),
                       unpacked.size());
        for (int step = 0; step < 100000 && !steps.step(64); ++step) {
        }
      };
      std::vector<uint8_t> truncated = packed;
      truncated.resize(truncated.size() / 2);
      REQUIRE_THROWS_AS(run(truncated, data.size()), std::runtime_error);
      REQUIRE_THROWS_AS(run(packed, data.size() - 100), std::runtime_error);
      REQUIRE_THROWS_AS(run(packed, data.size() + 100), std::runtime_error);
      REQUIRE_NOTHROW(run(packed, data.size()));
    }
  }
}

SCENARIO("A packed archive is read in place") {
  GIVEN("An archive with files and a bitmap") {
    const std::vector<uint8_t> data = archive();
    PackedFiles files(data.data(), data.size());

    THEN("Files read back unchanged") {
      REQUIRE(files.entries() == 4);
      REQUIRE(files.read("assets/text.txt") == bytes("hello"));
      REQUIRE(files.read("assets/music/song.s3m") == pattern(5000, 1));
      REQUIRE(files.read("./assets/music/empty.bin").empty());
    }

    THEN("Bitmaps keep their size, hotspot, palette and pixels") {
      const IndexedBitmap expected = bitmap();
      const IndexedBitmap loaded = files.loadBitmap("assets/0384/0384.bmp");
      REQUIRE(loaded.width == expected.width);
      REQUIRE(loaded.height == expected.height);
      REQUIRE(loaded.hotspotX == expected.hotspotX);
      REQUIRE(loaded.hotspotY == expected.hotspotY);
      REQUIRE(loaded.palette == expected.palette);
      REQUIRE(loaded.pixels == expected.pixels);
    }

    THEN("Directories exist and list their direct entries") {
      REQUIRE(files.exists("assets/music"));
      REQUIRE(files.exists("assets/music/"));
      REQUIRE_FALSE(files.exists("assets/mus"));
      REQUIRE(files.list("assets") ==
              std::vector<std::string>{"assets/0384", "assets/music",
                                       "assets/text.txt"});
      REQUIRE(files.list("assets/music") ==
              std::vector<std::string>{"assets/music/empty.bin",
                                       "assets/music/song.s3m"});
    }

    THEN("Walking a directory names the listed entries, in order") {
      const auto walked = [&files](const std::string &directory) {
        const auto walk = files.walk(directory);
        std::vector<std::string> names;
        std::string_view name;
        while (walk->next(name)) {
          names.emplace_back(name);
        }
        REQUIRE_FALSE(walk->next(name));
        return names;
      };
      REQUIRE(walked("assets") ==
              std::vector<std::string>{"0384", "music", "text.txt"});
      REQUIRE(walked("./assets/music/") ==
              std::vector<std::string>{"empty.bin", "song.s3m"});
      REQUIRE(walked("assets/missing").empty());
    }

    THEN("Missing files and wrong kinds fail like the other file sources") {
      REQUIRE_THROWS_WITH(files.read("assets/missing"),
                          "Failed to open assets/missing");
      REQUIRE_THROWS_WITH(files.loadBitmap("assets/missing.bmp"),
                          "Failed to load bitmap: assets/missing.bmp");
      REQUIRE_THROWS_WITH(files.loadBitmap("assets/text.txt"),
                          "Failed to load bitmap: assets/text.txt");
      REQUIRE_THROWS_WITH(files.read("assets/0384/0384.bmp"),
                          "Failed to open assets/0384/0384.bmp");
    }
  }

  GIVEN("Data that is not an archive") {
    const std::vector<uint8_t> data(64, 0);
    THEN("It is rejected") {
      REQUIRE_THROWS_AS(PackedFiles(data.data(), data.size()),
                        std::runtime_error);
    }
  }
}

SCENARIO("A packed archive finds names that share long prefixes") {
  GIVEN("Banks whose names extend each other, with many numbered files") {
    const std::vector<std::string> banks = {"s5", "s50", "s500", "s50x"};
    std::set<std::string> names;
    for (const std::string &bank : banks) {
      for (int index = 0; index < 150; ++index) {
        names.insert(imagePath(bank, index));
      }
    }
    names.insert("assets/s50/s50_sam2_13160Hz.wav");
    names.insert("assets/s50/s50_sam20_8000Hz.wav");
    names.insert("assets/s50/s50_sam2_notes.txt");
    names.insert("assets/s50.bmp");
    names.insert("assets/s5");
    packedArchive::ArchiveWriter writer;
    for (const std::string &name : names) {
      writer.addFile(name, bytes(name));
    }
    const std::vector<uint8_t> data = writer.finish();
    PackedFiles files(data.data(), data.size());

    std::vector<std::string> shuffled(names.begin(), names.end());
    std::mt19937 random(7);
    std::shuffle(shuffled.begin(), shuffled.end(), random);

    THEN("Every file reads back in any order") {
      for (const std::string &name : shuffled) {
        CAPTURE(name);
        REQUIRE(files.exists(name));
        REQUIRE(files.read(name) == bytes(name));
      }
    }

    THEN("Neighbouring names exist only as files or directories") {
      const auto expected = [&names](std::string probe) {
        while (!probe.empty() && probe.back() == '/') {
          probe.pop_back();
        }
        if (names.count(probe) != 0) {
          return true;
        }
        const auto after = names.lower_bound(probe + "/");
        return after != names.end() &&
               after->compare(0, probe.size() + 1, probe + "/") == 0;
      };
      for (const std::string &name : shuffled) {
        std::vector<std::string> probes = {
            name.substr(0, name.size() - 1), name + "a", name + "/",
            name.substr(0, name.find_last_of('/')),
            name.substr(0, name.find_last_of('/') + 2)};
        std::string lower = name;
        --lower.back();
        probes.push_back(lower);
        std::string higher = name;
        ++higher.back();
        probes.push_back(higher);
        std::string middle = name;
        ++middle[middle.size() / 2];
        probes.push_back(middle);
        for (const std::string &probe : probes) {
          CAPTURE(probe);
          REQUIRE(files.exists(probe) == expected(probe));
        }
      }
      REQUIRE(files.exists("assets/s5"));
      REQUIRE(files.exists("assets/s50"));
      REQUIRE_FALSE(files.exists("assets/s"));
      REQUIRE_FALSE(files.exists("assets/s50/s50_150.bmp"));
      REQUIRE_FALSE(files.exists("assets/s500/s500_07.bmp"));
    }

    THEN("A sample is found by its number and extension") {
      REQUIRE(samplePath(files, "s50", 2) == "assets/s50/s50_sam2_13160Hz.wav");
      REQUIRE(samplePath(files, "s50", 20) ==
              "assets/s50/s50_sam20_8000Hz.wav");
      REQUIRE(samplePath(files, "s50", 3).empty());
      REQUIRE(samplePath(files, "s500", 2).empty());
    }
  }
}

SCENARIO("Packed bitmaps and files are read in steps") {
  GIVEN("A picture larger than one step") {
    IndexedBitmap picture;
    picture.width = 320;
    picture.height = 100;
    picture.hotspotX = 3;
    picture.hotspotY = -2;
    for (uint16_t color = 0; color < 32; ++color) {
      picture.palette.push_back(static_cast<uint16_t>(color * 0x111 & 0xFFF));
    }
    picture.pixels = pattern(32000, 5);
    packedArchive::ArchiveWriter writer;
    writer.addBitmap("assets/big.bmp", picture);
    writer.addFile("assets/big.bin", pattern(40000, 6));
    writer.addFile("assets/empty.bin", {});
    const std::vector<uint8_t> data = writer.finish();
    PackedFiles files(data.data(), data.size());

    THEN("It takes several steps and matches loading it at once") {
      IndexedBitmap stepped;
      const auto load = files.beginBitmap("assets/big.bmp");
      int steps = 1;
      while (!load->step(stepped)) {
        ++steps;
      }
      REQUIRE(steps > 2);
      const IndexedBitmap whole = files.loadBitmap("assets/big.bmp");
      REQUIRE(stepped.width == whole.width);
      REQUIRE(stepped.height == whole.height);
      REQUIRE(stepped.hotspotX == whole.hotspotX);
      REQUIRE(stepped.hotspotY == whole.hotspotY);
      REQUIRE(stepped.palette == whole.palette);
      REQUIRE(stepped.pixels == picture.pixels);
    }

    THEN("A missing bitmap fails like loading it at once") {
      IndexedBitmap bitmap;
      const auto load = files.beginBitmap("assets/missing.bmp");
      REQUIRE_THROWS_WITH(load->step(bitmap),
                          "Failed to load bitmap: assets/missing.bmp");
    }

    THEN("A file takes several steps and matches reading it at once") {
      std::vector<uint8_t> stepped;
      const auto load = files.beginRead("assets/big.bin");
      int steps = 1;
      while (!load->step(stepped)) {
        ++steps;
      }
      REQUIRE(steps > 2);
      REQUIRE(stepped == files.read("assets/big.bin"));
      REQUIRE(stepped == pattern(40000, 6));
    }

    THEN("An empty file reads in a step") {
      std::vector<uint8_t> bytes{1, 2};
      const auto load = files.beginRead("assets/empty.bin");
      int steps = 1;
      while (!load->step(bytes)) {
        ++steps;
      }
      REQUIRE(steps <= 2);
      REQUIRE(bytes.empty());
    }

    THEN("Missing files and bitmaps fail like reading them at once") {
      std::vector<uint8_t> bytes;
      REQUIRE_THROWS_WITH(files.beginRead("assets/missing")->step(bytes),
                          "Failed to open assets/missing");
      REQUIRE_THROWS_WITH(files.beginRead("assets/big.bmp")->step(bytes),
                          "Failed to open assets/big.bmp");
    }
  }
}

SCENARIO("Corrupt LZ4 sequences are refused") {
  GIVEN("Blocks cut inside a sequence or pointing before the start") {
    const std::vector<std::vector<uint8_t>> blocks = {{0xF0},
                                                      {0x10, 'a', 0x01},
                                                      {0x10, 'a', 0x00, 0x00},
                                                      {0x10, 'a', 0x02, 0x00}};

    THEN("Each fails instead of reading or writing out of bounds") {
      for (const std::vector<uint8_t> &block : blocks) {
        CAPTURE(block);
        std::vector<uint8_t> unpacked(32, 0xAA);
        REQUIRE_THROWS_WITH(decompressLz4(block.data(), block.size(),
                                          unpacked.data(), unpacked.size()),
                            "Corrupt LZ4 data");
        REQUIRE(std::count(unpacked.begin() + 1, unpacked.end(), 0xAA) == 31);
      }
    }
  }
}

namespace {

constexpr auto TEXT = "assets/text.txt";
constexpr auto SONG = "assets/music/song.s3m";
constexpr auto PICTURE = "assets/0384/0384.bmp";
constexpr auto NOISE = "assets/noise.bmp";

IndexedBitmap noiseBitmap() {
  IndexedBitmap picture;
  picture.width = 8;
  picture.height = 4;
  picture.palette = {0x000, 0xFFF};
  for (int at = 0; at < picture.width * picture.height; ++at) {
    picture.pixels.push_back(static_cast<uint8_t>(at * 37 + 5));
  }
  return picture;
}

std::vector<uint8_t> corruptible() {
  packedArchive::ArchiveWriter writer;
  writer.addFile(SONG, pattern(5000, 1));
  writer.addFile(TEXT, bytes("hello"));
  writer.addBitmap(PICTURE, bitmap());
  writer.addBitmap(NOISE, noiseBitmap());
  return writer.finish();
}

void putLong(std::vector<uint8_t> &data, std::size_t at, uint32_t value) {
  data[at] = static_cast<uint8_t>(value >> 24);
  data[at + 1] = static_cast<uint8_t>(value >> 16);
  data[at + 2] = static_cast<uint8_t>(value >> 8);
  data[at + 3] = static_cast<uint8_t>(value);
}

std::size_t entryField(const std::vector<uint8_t> &data,
                       const std::string &name, std::size_t field) {
  const std::size_t count =
      packed::readLong(data.data() + packed::COUNT_OFFSET);
  for (std::size_t index = 0; index < count; ++index) {
    const std::size_t at = packed::HEADER_SIZE + index * packed::ENTRY_SIZE;
    const char *entry = reinterpret_cast<const char *>(
        data.data() + packed::readLong(data.data() + at + packed::NAME_OFFSET));
    if (name == entry) {
      return at + field;
    }
  }
  throw std::runtime_error("No entry " + name);
}

std::size_t dataOf(const std::vector<uint8_t> &data, const std::string &name) {
  return packed::readLong(data.data() +
                          entryField(data, name, packed::DATA_OFFSET));
}

constexpr auto CORRUPT = "Corrupt asset archive";

} // namespace

SCENARIO("A corrupt packed archive is refused instead of read out of bounds") {
  GIVEN("A packed archive with files and bitmaps") {
    std::vector<uint8_t> data = corruptible();

    THEN("The intact archive reads") {
      PackedFiles files(data.data(), data.size());
      REQUIRE(files.read(TEXT) == bytes("hello"));
      REQUIRE(files.loadBitmap(NOISE).pixels == noiseBitmap().pixels);
    }

    WHEN("The archive is cut short") {
      const std::size_t cut = data.size() - packed::DATA_ALIGNMENT;

      THEN("It is refused when opened") {
        REQUIRE_THROWS_WITH(PackedFiles(data.data(), cut), CORRUPT);
      }
    }

    WHEN("It claims more entries than its table holds") {
      putLong(data, packed::COUNT_OFFSET, 100000);

      THEN("It is refused when opened") {
        REQUIRE_THROWS_WITH(PackedFiles(data.data(), data.size()), CORRUPT);
      }
    }

    WHEN("A name points past the end") {
      putLong(data, entryField(data, TEXT, packed::NAME_OFFSET),
              static_cast<uint32_t>(data.size()));
      PackedFiles files(data.data(), data.size());

      THEN("Looking names up fails") {
        REQUIRE_THROWS_WITH(files.exists(TEXT), CORRUPT);
      }
    }

    WHEN("A file's data runs past the end") {
      putLong(data, entryField(data, TEXT, packed::DATA_OFFSET),
              static_cast<uint32_t>(data.size() - 2));
      PackedFiles files(data.data(), data.size());

      THEN("Reading it fails whether at once or in steps") {
        REQUIRE_THROWS_WITH(files.read(TEXT), CORRUPT);
        std::vector<uint8_t> stepped;
        REQUIRE_THROWS_WITH(files.beginRead(TEXT)->step(stepped), CORRUPT);
      }
    }

    WHEN("A file stored as is claims another size") {
      putLong(data, entryField(data, TEXT, packed::UNPACKED_SIZE_OFFSET), 6);
      PackedFiles files(data.data(), data.size());

      THEN("Reading it fails whether at once or in steps") {
        REQUIRE_THROWS_WITH(files.read(TEXT), CORRUPT);
        std::vector<uint8_t> stepped;
        REQUIRE_THROWS_WITH(files.beginRead(TEXT)->step(stepped), CORRUPT);
      }
    }

    WHEN("A compressed file claims more bytes than it unpacks to") {
      putLong(data, entryField(data, SONG, packed::UNPACKED_SIZE_OFFSET), 5100);
      PackedFiles files(data.data(), data.size());

      THEN("Reading it fails in the unpacking") {
        REQUIRE_THROWS_WITH(files.read(SONG), "Corrupt LZ4 data");
      }
    }

    WHEN("A bitmap claims more colours than it stores") {
      const std::size_t header = dataOf(data, PICTURE);
      data[header + packed::BITMAP_COLORS_OFFSET] = 0x7F;
      PackedFiles files(data.data(), data.size());

      THEN("Loading it fails whether at once or in steps") {
        REQUIRE_THROWS_WITH(files.loadBitmap(PICTURE), CORRUPT);
        IndexedBitmap stepped;
        REQUIRE_THROWS_WITH(files.beginBitmap(PICTURE)->step(stepped), CORRUPT);
      }
    }

    WHEN("A bitmap stored as is claims another size") {
      const std::size_t header = dataOf(data, NOISE);
      data[header + packed::BITMAP_WIDTH_OFFSET + 1] = 9;
      PackedFiles files(data.data(), data.size());

      THEN("Loading it fails whether at once or in steps") {
        REQUIRE_THROWS_WITH(files.loadBitmap(NOISE), CORRUPT);
        IndexedBitmap stepped;
        REQUIRE_THROWS_WITH(files.beginBitmap(NOISE)->step(stepped), CORRUPT);
      }
    }
  }
}

SCENARIO("A directory in a packed archive is not a bitmap") {
  GIVEN("An archive with a bitmap inside a directory") {
    const std::vector<uint8_t> data = archive();
    PackedFiles files(data.data(), data.size());

    THEN("Loading the directory as a bitmap fails like opening it") {
      REQUIRE_THROWS_WITH(files.loadBitmap("assets/0384"),
                          "Failed to open assets/0384");
      IndexedBitmap stepped;
      REQUIRE_THROWS_WITH(files.beginBitmap("assets/0384/")->step(stepped),
                          "Failed to open assets/0384/");
    }
  }
}
