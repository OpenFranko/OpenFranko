#include "../../lib/argumentParser/ArgumentParser.h"
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
  std::string fileId = std::filesystem::path(inputPath).filename().string();

  std::string outDir = ".";
  const auto outputOptional = parser.getCmdOption("-o");
  if (outputOptional.has_value())
    outDir = outputOptional.value();

  const auto paletteOptional = parser.getCmdOption("-p");
  std::vector<uint16_t> palette;
  if (paletteOptional.has_value()) {
    const std::string &p = paletteOptional.value();
    namespace pal = converter::spriteSheet::palettes;
    if (p == "sunset")
      palette = {pal::SUNSET.begin(), pal::SUNSET.end()};
    else if (p == "story")
      palette = {pal::STORY.begin(), pal::STORY.end()};
    else if (p == "menu")
      palette = {pal::MENU.begin(), pal::MENU.end()};
    else if (p == "menu35")
      palette = {pal::MENU_35.begin(), pal::MENU_35.end()};
    else if (p == "cemetery")
      palette = {pal::CEMETERY.begin(), pal::CEMETERY.end()};
    else
      palette = {pal::LEVEL.begin(), pal::LEVEL.end()};
  } else {
    palette = converter::spriteSheet::selectPalette(fileId);
  }

  try {
    auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;

    auto dec = decompressor::backwardLZ77::decompress(raw);
    std::cerr << "Decompressed to " << dec.size() << " bytes" << std::endl;

    auto header = converter::spriteSheet::parseHeader(dec);
    std::cerr << "Sprite bank: " << header.count << " sprites, max "
              << header.maxWidth << "x" << header.maxHeight << std::endl;

    std::filesystem::create_directories(outDir);

    auto sprites = converter::spriteSheet::convertToIndividual(dec, palette);
    int written = 0;
    for (int i = 0; i < static_cast<int>(sprites.size()); i++) {
      if (sprites[i].bmpData.empty()) {
        std::cerr << "Skipped sprite " << i << ": " << sprites[i].error
                  << std::endl;
        continue;
      }
      char buf[64];
      snprintf(buf, sizeof(buf), "%s/%s_%03d.bmp", outDir.c_str(),
               fileId.c_str(), i);
      std::string bmpPath(buf);
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
