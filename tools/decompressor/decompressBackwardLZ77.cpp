#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/gameData/gameData.h"
#include "../../lib/decompressor/backwardLZ77/backwardLZ77.h"
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
    std::cerr << "Unpacks a Franko 1.0 data file, or the squashed block of a "
                 "version 1.2 data file."
              << std::endl;
    return 1;
  }

  const std::string inputPath = inputOption.value();
  const auto outputOption = parser.option("-o");

  try {
    const auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;
    const std::string fileName =
        std::filesystem::path(inputPath).filename().string();
    const auto decompressed =
        converter::gameData::version12::find(fileName) == nullptr
            ? decompressor::backwardLZ77::decompress(raw)
            : converter::fileContainer::unsquashVersion12(fileName, raw);
    std::cerr << "Decompressed to " << decompressed.size() << " bytes"
              << std::endl;

    const std::string outputPath = outputOption.value_or(fileName + ".dec");
    filesystem::writeFile::writeFile(outputPath, decompressed);
    std::cerr << "Wrote " << outputPath << " (" << decompressed.size()
              << " bytes)" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
