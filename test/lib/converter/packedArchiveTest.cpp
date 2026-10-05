#include "../../../lib/converter/packedArchive/packedArchive.h"

#include "../../../lib/bmpWriter/bmpWriter.h"
#include "../../../src/engine/assets/PackedFiles.h"
#include "../../TemporaryPath.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace openfranko::lib::converter::packedArchive;
using openfranko::lib::bmpWriter::pixelsToBmp;
using openfranko::src::engine::assets::PackedFiles;
using openfranko::src::systems::graphics::IndexedBitmap;
using openfranko::test::TemporaryPath;

namespace {

void writeBytes(const std::filesystem::path &path,
                const std::vector<uint8_t> &data) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream file(path, std::ios::binary);
  file.write(reinterpret_cast<const char *>(data.data()),
             static_cast<std::streamsize>(data.size()));
}

std::vector<uint8_t> repeatingBytes(std::size_t size) {
  std::vector<uint8_t> data(size);
  for (std::size_t at = 0; at < size; ++at) {
    data[at] = static_cast<uint8_t>(at % 5);
  }
  return data;
}

IndexedBitmap tinyBitmap() {
  IndexedBitmap bitmap;
  bitmap.width = 1;
  bitmap.height = 1;
  bitmap.pixels = {0};
  bitmap.palette = {0x000};
  return bitmap;
}

} // namespace

SCENARIO("packDirectory packs every file under a directory by its relative "
         "path") {
  GIVEN("A directory with a text file, a nested music file, a bitmap and an "
        "empty folder") {
    const TemporaryPath root("openFrankoPackDirectory");
    const std::vector<uint8_t> text = {'h', 'i'};
    const std::vector<uint8_t> music = repeatingBytes(600);
    const std::vector<uint8_t> pixels = {0, 1, 2, 3, 3, 2};
    const std::vector<uint16_t> palette = {0x000, 0xF00, 0x0F0, 0x00F};
    writeBytes(root.path() / "text.txt", text);
    writeBytes(root.path() / "music" / "song.s3m", music);
    writeBytes(root.path() / "pictures" / "logo.bmp",
               pixelsToBmp(3, 2, pixels.data(), palette.data(), 4));
    std::filesystem::create_directories(root.path() / "empty");

    WHEN("It is packed under the prefix assets") {
      const std::vector<uint8_t> archive =
          packDirectory(root.path().string(), "assets");
      PackedFiles files(archive.data(), archive.size());

      THEN("Each file is stored under the prefix and its relative path") {
        REQUIRE(files.entries() == 3);
        REQUIRE(files.exists("assets/text.txt"));
        REQUIRE(files.exists("assets/music/song.s3m"));
        REQUIRE(files.exists("assets/pictures/logo.bmp"));
        REQUIRE_FALSE(files.exists("assets/empty"));
      }

      THEN("Files read back unchanged") {
        REQUIRE(files.read("assets/text.txt") == text);
        REQUIRE(files.read("assets/music/song.s3m") == music);
      }

      THEN("The bitmap is stored as an indexed bitmap with its palette") {
        const IndexedBitmap bitmap =
            files.loadBitmap("assets/pictures/logo.bmp");
        REQUIRE(bitmap.width == 3);
        REQUIRE(bitmap.height == 2);
        REQUIRE(bitmap.pixels == pixels);
        REQUIRE(bitmap.palette == palette);
      }
    }

    WHEN("It is packed without a prefix") {
      const std::vector<uint8_t> archive =
          packDirectory(root.path().string(), "");
      PackedFiles files(archive.data(), archive.size());

      THEN("Files are stored by their relative paths alone") {
        REQUIRE(files.entries() == 3);
        REQUIRE(files.read("text.txt") == text);
        REQUIRE(files.read("music/song.s3m") == music);
        REQUIRE(files.loadBitmap("pictures/logo.bmp").pixels == pixels);
      }
    }

    WHEN("It is packed twice") {
      THEN("Both archives are identical") {
        REQUIRE(packDirectory(root.path().string(), "assets") ==
                packDirectory(root.path().string(), "assets"));
      }
    }
  }

  GIVEN("A regular file and a path that does not exist") {
    const TemporaryPath file("openFrankoPackDirectoryFile");
    writeBytes(file.path(), {1});
    const std::string missing = (file.path() / "missing").string();

    THEN("Neither is packed") {
      REQUIRE_THROWS_WITH(packDirectory(file.path().string(), ""),
                          "Not a directory: " + file.path().string());
      REQUIRE_THROWS_WITH(packDirectory(missing, ""),
                          "Not a directory: " + missing);
    }
  }

  GIVEN("A directory with a .bmp file that is not a bitmap") {
    const TemporaryPath root("openFrankoPackDirectoryBroken");
    writeBytes(root.path() / "broken.bmp", {'n', 'o', 'p', 'e'});

    THEN("Packing fails with the bitmap reader's reason") {
      REQUIRE_THROWS_WITH(packDirectory(root.path().string(), ""),
                          "Not a bitmap");
    }
  }
}

SCENARIO("ArchiveWriter refuses two entries with the same name") {
  GIVEN("A writer given a file and then a bitmap under the same name") {
    ArchiveWriter writer;
    writer.addFile("assets/a.bin", {1, 2, 3});
    writer.addFile("assets/b.bin", {4});
    writer.addBitmap("assets/a.bin", tinyBitmap());

    THEN("finish throws, naming the entry") {
      REQUIRE_THROWS_WITH(writer.finish(),
                          "Duplicate archive entry: assets/a.bin");
    }
  }
}
