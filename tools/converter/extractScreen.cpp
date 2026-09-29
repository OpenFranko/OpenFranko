#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/amosCompact/amosCompact.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/gameData/gameData.h"
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
    std::cerr << "Usage: " << argv[0] << " -i <input_file> [-o <output_file>]"
              << std::endl;
    std::cerr << "Converts a Franko screen package (type 0x0201, or a version "
                 "1.2 p file holding a screen) to BMP."
              << std::endl;
    return 1;
  }

  const std::string inputPath = inputOption.value();
  const auto outputOption = parser.option("-o");

  try {
    const auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;
    const auto resource = converter::fileContainer::unpack(
        std::filesystem::path(inputPath).filename().string(), raw);
    const std::string &fileId = resource.fileId;
    if (resource.resourceType !=
        converter::gameData::resourceTypes::SCREEN_PACKAGE) {
      std::cerr << "Warning: " << fileId
                << " is not a screen package (type 0x0201, or a version 1.2 p "
                   "file holding a screen)"
                << std::endl;
    }

    const auto bmp = converter::amosCompact::decompress(resource.data);
    std::cerr << "Decompressed to " << bmp.size() << " bytes" << std::endl;
    const std::string outputPath = outputOption.value_or(fileId + ".bmp");
    filesystem::writeFile::writeFile(outputPath, bmp);
    std::cerr << "Wrote " << outputPath << " (" << bmp.size() << " bytes)"
              << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
