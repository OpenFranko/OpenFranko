#include "../../../../src/engine/assets/PackedFiles.h"
#include "../../../../lib/converter/packedArchive/lz4Compressor.h"
#include "../../../../lib/converter/packedArchive/packedArchive.h"
#include "../../../../src/engine/assets/Assets.h"
#include "../../../../src/engine/assets/Lz4.h"

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
