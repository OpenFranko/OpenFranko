#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/spriteSheet/spriteSheet.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <filesystem>
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");

  if (!inputOptional.has_value()) {
    std::cerr << "Usage: " << argv[0]
              << " -i <input_file> [-o <output.bmp>] [-p <palette>]"
              << std::endl;
    std::cerr << "Palettes: level (default), sunset, story, menu, menu35, "
                 "cemetery"
              << std::endl;
    return 1;
  }

  std::string inputFilePath = inputOptional.value();
  const auto outputOptional = parser.getCmdOption("-o");
  const auto paletteOptional = parser.getCmdOption("-p");

  try {
    auto compressedData = filesystem::readFile::readFile(inputFilePath);
    auto resource = converter::fileContainer::unpack(
        std::filesystem::path(inputFilePath).filename().string(),
        compressedData);
    const std::string &fileId = resource.fileId;

    std::string outputFilePath = outputOptional.value_or(fileId + "_sheet.bmp");
    auto palette =
        paletteOptional.has_value()
            ? converter::spriteSheet::palettes::byName(paletteOptional.value())
            : converter::spriteSheet::selectPalette(fileId);

    std::cerr << "Decompressing " << inputFilePath << " (" << compressedData.size()
              << " bytes)..." << std::endl;
    const auto &decompressedData = resource.data;
    std::cerr << "Decompressed to " << decompressedData.size() << " bytes"
              << std::endl;

    auto header = converter::spriteSheet::parseHeader(decompressedData);
    std::cerr << "Sprite bank: " << header.count << " sprites, max "
              << header.maxWidth << "x" << header.maxHeight << ", "
              << header.numColors << " colors" << std::endl;

    auto sheet =
        converter::spriteSheet::convertToSheet(decompressedData, palette);
    for (size_t i = 0; i < sheet.spriteErrors.size(); i++) {
      if (!sheet.spriteErrors[i].empty()) {
        std::cerr << "Skipped sprite " << i << ": " << sheet.spriteErrors[i]
                  << std::endl;
      }
    }
    filesystem::writeFile::writeFile(outputFilePath, sheet.bmpData);
    std::cerr << "Wrote " << outputFilePath << " (" << sheet.bmpData.size()
              << " bytes)" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
