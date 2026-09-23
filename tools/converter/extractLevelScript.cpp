#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/gameData/gameData.h"
#include "../../lib/converter/levelScript/levelScript.h"
#include "../../lib/decompressor/backwardLZ77/backwardLZ77.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <algorithm>
#include <iostream>
#include <string_view>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");
  if (!inputOptional.has_value()) {
    std::cerr << "Usage: " << argv[0] << " -i <input_file> [-o <output.json>]"
              << std::endl;
    std::cerr << "Converts a Franko level script (files 0385-0387) to JSON."
              << std::endl;
    return 1;
  }

  std::string inputPath = inputOptional.value();
  const auto outputOptional = parser.getCmdOption("-o");

  try {
    auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;
    std::string fileId = converter::fileContainer::fileIdToHex(
        converter::fileContainer::parseFooter(raw).fileId);
    std::string outputPath = outputOptional.value_or(fileId + ".json");

    const auto &levelFiles = converter::gameData::fileIds::LEVEL_FILES;
    if (std::find(levelFiles.begin(), levelFiles.end(),
                  std::string_view(fileId)) == levelFiles.end())
      std::cerr << "Warning: " << fileId
                << " is not one of the level script files (0385-0387)"
                << std::endl;

    auto dec = decompressor::backwardLZ77::decompress(raw);
    std::cerr << "Decompressed to " << dec.size() << " bytes" << std::endl;

    auto level = converter::levelScript::parse(dec);
    std::cerr << "Level script: " << level.lengthInColumns << " columns, "
              << level.waves.size() << " waves" << std::endl;

    auto json = converter::levelScript::toJson(level, fileId);
    filesystem::writeFile::writeFile(outputPath, json);
    std::cerr << "Wrote " << outputPath << " (" << json.size() << " bytes)"
              << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
