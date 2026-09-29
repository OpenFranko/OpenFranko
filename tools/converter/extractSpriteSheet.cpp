#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/gameData/gameData.h"
#include "../../lib/converter/gameData/palettes.h"
#include "../../lib/converter/spriteSheet/spriteSheet.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"

#include <filesystem>
#include <iostream>
#include <string>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOption = parser.option("-i");
  if (!inputOption.has_value()) {
    std::cerr << "Usage: " << argv[0]
              << " -i <input_file> [-o <output_file>] [-p <palette>]"
              << std::endl;
    std::cerr << "Converts a Franko sprite bank (type 0x0000, or a version "
                 "1.2 s file) to a BMP sprite sheet."
              << std::endl;
    std::cerr << "Palettes: level (default), sunset, story, menu, menu35, "
                 "cemetery"
              << std::endl;
    return 1;
  }

  const std::string inputPath = inputOption.value();
  const auto outputOption = parser.option("-o");
  const auto paletteOption = parser.option("-p");

  try {
    const auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;
    const auto resource = converter::fileContainer::unpack(
        std::filesystem::path(inputPath).filename().string(), raw);
    const std::string &fileId = resource.fileId;
    if (resource.resourceType != converter::gameData::resourceTypes::SPRITES) {
      std::cerr << "Warning: " << fileId
                << " is not a sprite bank (type 0x0000, or a version 1.2 s "
                   "file)"
                << std::endl;
    }

    const auto palette =
        paletteOption.has_value()
            ? converter::gameData::palettes::byName(paletteOption.value())
            : converter::gameData::palettes::selectPalette(fileId);

    const auto &decompressed = resource.data;
    std::cerr << "Decompressed to " << decompressed.size() << " bytes"
              << std::endl;

    const auto header = converter::spriteSheet::parseHeader(decompressed);
    std::cerr << "Sprite bank: " << header.count << " sprites, max "
              << header.maxWidth << "x" << header.maxHeight << ", "
              << header.numberOfColors << " colors" << std::endl;

    const auto sheet =
        converter::spriteSheet::convertToSheet(decompressed, palette);
    for (size_t i = 0; i < sheet.spriteErrors.size(); i++) {
      if (!sheet.spriteErrors[i].empty()) {
        std::cerr << "Skipped sprite " << i << ": " << sheet.spriteErrors[i]
                  << std::endl;
      }
    }
    const std::string outputPath = outputOption.value_or(fileId + "_sheet.bmp");
    filesystem::writeFile::writeFile(outputPath, sheet.data);
    std::cerr << "Wrote " << outputPath << " (" << sheet.data.size()
              << " bytes)" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
