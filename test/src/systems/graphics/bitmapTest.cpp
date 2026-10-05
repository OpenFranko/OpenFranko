#include "../../../../src/systems/graphics/Bitmap.h"

#include "../../../TemporaryPath.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace openfranko::src::systems::graphics;
using namespace openfranko::test;

namespace {

void put(std::vector<uint8_t> &file, std::size_t offset, int size,
         uint32_t value) {
  for (int i = 0; i < size; ++i) {
    file[offset + static_cast<std::size_t>(i)] =
        static_cast<uint8_t>(value >> (8 * i));
  }
}

struct BitmapFile {
  int width = 3;
  int height = 2;
  uint16_t hotX = 0;
  uint16_t hotY = 0;
  uint16_t bits = 8;
  uint32_t compression = 0;
  uint32_t colorsUsed = 2;
  std::vector<uint32_t> palette = {0x000000, 0xFF8800};
  std::vector<std::vector<uint8_t>> rows = {{0, 1, 0}, {1, 1, 0}};

  std::vector<uint8_t> bytes() const {
    const std::size_t stride = (static_cast<std::size_t>(width) + 3) / 4 * 4;
    const std::size_t pixelOffset = 14 + 40 + palette.size() * 4;
    std::vector<uint8_t> file(pixelOffset + stride * rows.size(), 0);
    file[0] = 'B';
    file[1] = 'M';
    put(file, 2, 4, static_cast<uint32_t>(file.size()));
    put(file, 6, 2, hotX);
    put(file, 8, 2, hotY);
    put(file, 10, 4, static_cast<uint32_t>(pixelOffset));
    put(file, 14, 4, 40);
    put(file, 18, 4, static_cast<uint32_t>(width));
    put(file, 22, 4, static_cast<uint32_t>(height));
    put(file, 26, 2, 1);
    put(file, 28, 2, bits);
    put(file, 30, 4, compression);
    put(file, 46, 4, colorsUsed);
    for (std::size_t i = 0; i < palette.size(); ++i) {
      put(file, 54 + i * 4, 3, palette[i]);
    }
    for (std::size_t row = 0; row < rows.size(); ++row) {
      const std::size_t stored = height > 0 ? rows.size() - 1 - row : row;
      for (std::size_t x = 0; x < rows[row].size(); ++x) {
        file[pixelOffset + stored * stride + x] = rows[row][x];
      }
    }
    return file;
  }
};

} // namespace

SCENARIO("readIndexedBitmap reads the 8-bit pictures the asset pipeline "
         "writes") {
  GIVEN("A 3 x 2 bottom-up bitmap with padded rows and a hot spot") {
    BitmapFile file;
    file.hotX = 5;
    file.hotY = 7;

    WHEN("It is read") {
      const IndexedBitmap bitmap = readIndexedBitmap(file.bytes());

      THEN("The rows come out top-down without their padding") {
        REQUIRE(bitmap.width == 3);
        REQUIRE(bitmap.height == 2);
        REQUIRE(bitmap.pixels == std::vector<uint8_t>{0, 1, 0, 1, 1, 0});
      }

      THEN("The hot spot is read from the two reserved header words") {
        REQUIRE(bitmap.hotspotX == 5);
        REQUIRE(bitmap.hotspotY == 7);
      }

      THEN("The palette holds the used colours as 12-bit Amiga colours") {
        REQUIRE(bitmap.palette == std::vector<uint16_t>{0x000, 0xF80});
      }
    }
  }

  GIVEN("A colour between two of the 17 channel steps") {
    BitmapFile file;
    file.palette = {0x0809FF, 0x191A00};

    THEN("Each channel rounds to the nearest nibble") {
      REQUIRE(readIndexedBitmap(file.bytes()).palette ==
              std::vector<uint16_t>{0x01F, 0x120});
    }
  }

  GIVEN("A top-down bitmap") {
    BitmapFile file;
    file.height = -2;

    THEN("Its rows keep their order") {
      REQUIRE(readIndexedBitmap(file.bytes()).pixels ==
              std::vector<uint8_t>{0, 1, 0, 1, 1, 0});
    }
  }

  GIVEN("A header that leaves the colour count at 0") {
    BitmapFile file;
    file.colorsUsed = 0;
    file.palette.resize(256, 0x111111);

    THEN("All 256 colours are read") {
      const IndexedBitmap bitmap = readIndexedBitmap(file.bytes());
      REQUIRE(bitmap.palette.size() == 256);
      REQUIRE(bitmap.palette[255] == 0x111);
    }
  }

  GIVEN("Files the game never uses") {
    THEN("Anything but an uncompressed 8-bit bitmap is refused") {
      BitmapFile fourBits;
      fourBits.bits = 4;
      REQUIRE_THROWS_AS(readIndexedBitmap(fourBits.bytes()),
                        std::runtime_error);

      BitmapFile compressed;
      compressed.compression = 1;
      REQUIRE_THROWS_AS(readIndexedBitmap(compressed.bytes()),
                        std::runtime_error);

      std::vector<uint8_t> notBitmap = BitmapFile().bytes();
      notBitmap[0] = 'P';
      REQUIRE_THROWS_AS(readIndexedBitmap(notBitmap), std::runtime_error);
    }

    THEN("A file cut short in its pixels is refused") {
      std::vector<uint8_t> file = BitmapFile().bytes();
      file.resize(file.size() - 2);
      REQUIRE_THROWS_AS(readIndexedBitmap(file), std::runtime_error);
    }
  }
}

SCENARIO("loadIndexedBitmap names the file it could not read") {
  GIVEN("A path with no file") {
    const TemporaryPath missing("openFrankoMissingPicture.bmp");
    const std::string path = missing.path().string();

    THEN("The error carries the path") {
      REQUIRE_THROWS_WITH(loadIndexedBitmap(path),
                          Catch::Matchers::ContainsSubstring(path));
    }
  }
}

SCENARIO("mirrored turns a picture round its hot spot") {
  GIVEN("A 3 x 2 picture with its hot spot one pixel in") {
    const IndexedBitmap picture{3, 2, 1, 1, {1, 2, 3, 4, 5, 6}, {0x123}};

    WHEN("It is mirrored") {
      const IndexedBitmap flipped = mirrored(picture);

      THEN("Each row reads backwards") {
        REQUIRE(flipped.pixels == std::vector<uint8_t>{3, 2, 1, 6, 5, 4});
      }

      THEN("The hot spot is measured from the other side") {
        REQUIRE(flipped.hotspotX == 2);
        REQUIRE(flipped.hotspotY == 1);
      }

      THEN("Its size and colours stay") {
        REQUIRE(flipped.width == 3);
        REQUIRE(flipped.height == 2);
        REQUIRE(flipped.palette == picture.palette);
      }
    }
  }
}

SCENARIO("readIndexedBitmap refuses headers it cannot use") {
  GIVEN("Bitmaps with an old header, no pixels or too many colours") {
    THEN("A header shorter than 40 bytes is refused") {
      std::vector<uint8_t> file = BitmapFile().bytes();
      put(file, 14, 4, 12);
      REQUIRE_THROWS_WITH(readIndexedBitmap(file),
                          Catch::Matchers::ContainsSubstring("header"));
    }

    THEN("A width or height of 0 is refused") {
      BitmapFile narrow;
      narrow.width = 0;
      narrow.rows = {{}, {}};
      REQUIRE_THROWS_WITH(readIndexedBitmap(narrow.bytes()),
                          Catch::Matchers::ContainsSubstring("Empty"));
      BitmapFile flat;
      flat.height = 0;
      flat.rows.clear();
      REQUIRE_THROWS_WITH(readIndexedBitmap(flat.bytes()),
                          Catch::Matchers::ContainsSubstring("Empty"));
    }

    THEN("More than 256 colours are refused") {
      BitmapFile file;
      file.colorsUsed = 257;
      REQUIRE_THROWS_WITH(readIndexedBitmap(file.bytes()),
                          Catch::Matchers::ContainsSubstring("colours"));
    }
  }
}

SCENARIO("loadIndexedBitmap reads a picture from a file") {
  GIVEN("A bitmap written to a file") {
    const TemporaryPath stored("openFrankoStoredPicture.bmp");
    BitmapFile file;
    file.hotX = 2;
    const std::vector<uint8_t> bytes = file.bytes();
    {
      std::ofstream stream(stored.path(), std::ios::binary);
      stream.write(reinterpret_cast<const char *>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()));
    }

    THEN("It is the picture readIndexedBitmap reads from the bytes") {
      const IndexedBitmap loaded = loadIndexedBitmap(stored.path().string());
      const IndexedBitmap read = readIndexedBitmap(bytes);
      REQUIRE(loaded.width == read.width);
      REQUIRE(loaded.height == read.height);
      REQUIRE(loaded.hotspotX == 2);
      REQUIRE(loaded.pixels == read.pixels);
      REQUIRE(loaded.palette == read.palette);
    }
  }

  GIVEN("A file that is not a bitmap") {
    const TemporaryPath stored("openFrankoNotAPicture.bmp");
    {
      std::ofstream stream(stored.path(), std::ios::binary);
      stream << "PK not a picture";
    }
    const std::string path = stored.path().string();

    THEN("The error names both the problem and the file") {
      REQUIRE_THROWS_WITH(loadIndexedBitmap(path),
                          Catch::Matchers::ContainsSubstring("Not a bitmap") &&
                              Catch::Matchers::ContainsSubstring(path));
    }
  }
}
