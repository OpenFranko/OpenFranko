#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/gameData/gameData.h"
#include "../../lib/converter/gameData/palettes.h"
#include "../../lib/converter/spriteSheet/spriteSheet.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOption = parser.option("-i");
  if (!inputOption.has_value()) {
    std::cerr << "Usage: " << argv[0]
              << " -i <input_file> [-o <output_dir>] [-p <palette>]"
              << std::endl;
    std::cerr << "Extracts individual sprites from a Franko sprite bank (type "
                 "0x0000, or a version 1.2 s file)."
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
              << header.maxWidth << "x" << header.maxHeight << std::endl;

    const std::string outputDir = outputOption.value_or("extracted");
    std::filesystem::create_directories(outputDir);

    auto sprites =
        converter::spriteSheet::convertToIndividual(decompressed, palette);
    if (!paletteOption.has_value()) {
      converter::spriteSheet::applySpritePaletteFixes(fileId, sprites);
      const std::string screen(converter::spriteSheet::paletteScreen(fileId));
      if (!screen.empty()) {
        const auto screenPath =
            std::filesystem::path(inputPath).parent_path() / screen;
        try {
          const auto bank = converter::fileContainer::unpack(
              screen, filesystem::readFile::readFile(screenPath.string()));
          converter::spriteSheet::applyScreenPalette(fileId, bank.data,
                                                     sprites);
        } catch (const std::exception &e) {
          std::cerr << "Warning: " << e.what() << ": the sprites shown on "
                    << screen << " keep the bank's palette" << std::endl;
        }
      }
    }
    int written = 0;
    for (int i = 0; i < static_cast<int>(sprites.size()); ++i) {
      if (sprites[i].data.empty()) {
        std::cerr << "Skipped sprite " << i << ": " << sprites[i].error
                  << std::endl;
        continue;
      }
      char name[32];
      snprintf(name, sizeof(name), "%s_%03d.bmp", fileId.c_str(), i);
      const std::string path = outputDir + "/" + name;
      filesystem::writeFile::writeFile(path, sprites[i].data);
      std::cerr << "Wrote " << path << " (" << sprites[i].data.size()
                << " bytes)" << std::endl;
      ++written;
    }

    if (written == 0) {
      std::cerr << "No sprites extracted." << std::endl;
    } else {
      std::cerr << "Wrote " << written << " sprites to " << outputDir
                << std::endl;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
