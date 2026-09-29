#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/codeCards/codeCards.h"
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
    std::cerr << "Converts the Franko copy protection code cards (file 0384, "
                 "or p0 of version 1.2) to JSON."
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
    if (converter::gameData::version10Id(fileId) !=
        converter::gameData::fileIds::CODE_CARDS) {
      std::cerr << "Warning: " << fileId
                << " is not the code card file (0384, or p0 of version 1.2)"
                << std::endl;
    }

    const auto &decompressed = resource.data;
    std::cerr << "Decompressed to " << decompressed.size() << " bytes"
              << std::endl;

    const bool version12 =
        converter::gameData::version12::find(fileId) != nullptr;
    const auto cards = converter::codeCards::parse(
        decompressed, version12
                          ? converter::codeCards::consts::VERSION12_CARD_SIZE
                          : converter::codeCards::consts::CARD_SIZE);
    const auto json = converter::codeCards::toJson(cards);
    const std::string outputPath =
        outputOption.value_or(fileId + "_codecards.json");
    filesystem::writeFile::writeFile(outputPath, json);
    std::cerr << "Wrote " << outputPath << " (" << json.size() << " bytes)"
              << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
