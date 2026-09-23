#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/spriteSheet/spriteSheet.h"
#include "../../lib/decompressor/backwardLZ77/backwardLZ77.h"
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
  std::string fileId =
      std::filesystem::path(inputFilePath).filename().string();

  std::string outputFilePath = fileId + "_sheet.bmp";
  const auto outputOptional = parser.getCmdOption("-o");
  if (outputOptional.has_value()) {
    outputFilePath = outputOptional.value();
  }

  const auto paletteOptional = parser.getCmdOption("-p");
  std::vector<uint16_t> palette;
  if (paletteOptional.has_value()) {
    const std::string &palName = paletteOptional.value();
    if (palName == "sunset") {
      palette = {converter::spriteSheet::palettes::SUNSET.begin(),
                 converter::spriteSheet::palettes::SUNSET.end()};
    } else if (palName == "story") {
      palette = {converter::spriteSheet::palettes::STORY.begin(),
                 converter::spriteSheet::palettes::STORY.end()};
    } else if (palName == "menu") {
      palette = {converter::spriteSheet::palettes::MENU.begin(),
                 converter::spriteSheet::palettes::MENU.end()};
    } else if (palName == "menu35") {
      palette = {converter::spriteSheet::palettes::MENU_35.begin(),
                 converter::spriteSheet::palettes::MENU_35.end()};
    } else if (palName == "cemetery") {
      palette = {converter::spriteSheet::palettes::CEMETERY.begin(),
                 converter::spriteSheet::palettes::CEMETERY.end()};
    } else {
      palette = {converter::spriteSheet::palettes::LEVEL.begin(),
                 converter::spriteSheet::palettes::LEVEL.end()};
    }
  } else {
    palette = converter::spriteSheet::selectPalette(fileId);
  }

  try {
    auto compressedData = filesystem::readFile::readFile(inputFilePath);

    std::cerr << "Decompressing " << inputFilePath << " (" << compressedData.size()
              << " bytes)..." << std::endl;
    auto decompressedData =
        decompressor::backwardLZ77::decompress(compressedData);
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
