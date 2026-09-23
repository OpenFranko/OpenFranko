#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/spriteSheet/spriteSheet.h"
#include "../../lib/decompressor/backwardLZ77/backwardLZ77.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <cstdio>
#include <filesystem>
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");
  if (!inputOptional.has_value()) {
    std::cerr << "Usage: " << argv[0]
              << " -i <input_file> [-o <output_dir>] [-p <palette>]"
              << std::endl;
    std::cerr << "Extracts individual sprites from a Franko sprite bank (type "
                 "0x0000)."
              << std::endl;
    std::cerr << "Palettes: level (default), sunset, story, menu, menu35, "
                 "cemetery"
              << std::endl;
    return 1;
  }

  std::string inputPath = inputOptional.value();

  std::string outDir = ".";
  const auto outputOptional = parser.getCmdOption("-o");
  if (outputOptional.has_value())
    outDir = outputOptional.value();

  const auto paletteOptional = parser.getCmdOption("-p");

  try {
    auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;
    std::string fileId = converter::fileContainer::fileIdToHex(
        converter::fileContainer::parseFooter(raw).fileId);

    auto palette =
        paletteOptional.has_value()
            ? converter::spriteSheet::palettes::byName(paletteOptional.value())
            : converter::spriteSheet::selectPalette(fileId);

    auto dec = decompressor::backwardLZ77::decompress(raw);
    std::cerr << "Decompressed to " << dec.size() << " bytes" << std::endl;

    auto header = converter::spriteSheet::parseHeader(dec);
    std::cerr << "Sprite bank: " << header.count << " sprites, max "
              << header.maxWidth << "x" << header.maxHeight << std::endl;

    std::filesystem::create_directories(outDir);

    auto sprites = converter::spriteSheet::convertToIndividual(dec, palette);
    if (!paletteOptional.has_value()) {
      converter::spriteSheet::applySpritePaletteFixes(fileId, sprites);
    }
    int written = 0;
    for (int i = 0; i < static_cast<int>(sprites.size()); i++) {
      if (sprites[i].bmpData.empty()) {
        std::cerr << "Skipped sprite " << i << ": " << sprites[i].error
                  << std::endl;
        continue;
      }
      char name[32];
      snprintf(name, sizeof(name), "%s_%03d.bmp", fileId.c_str(), i);
      std::string bmpPath = outDir + "/" + name;
      filesystem::writeFile::writeFile(bmpPath, sprites[i].bmpData);
      written++;
    }
    std::cerr << "Wrote " << written << " sprites to " << outDir << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
