#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/codeCards/codeCards.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/gameData/gameData.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <filesystem>
#include <iostream>
#include <string_view>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");
  if (!inputOptional.has_value()) {
    std::cerr << "Usage: " << argv[0] << " -i <input_file> [-o <output.json>]"
              << std::endl;
    std::cerr << "Converts the Franko copy protection code cards (file 0384, "
                 "or p0 of version 1.2) to JSON."
              << std::endl;
    return 1;
  }

  std::string inputPath = inputOptional.value();
  const auto outputOptional = parser.getCmdOption("-o");

  try {
    auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;
    auto resource = converter::fileContainer::unpack(
        std::filesystem::path(inputPath).filename().string(), raw);
    const std::string &fileId = resource.fileId;
    std::string outputPath =
        outputOptional.value_or(fileId + "_codecards.json");

    if (converter::gameData::version10Id(fileId) !=
        converter::gameData::fileIds::CODE_CARDS)
      std::cerr << "Warning: " << fileId
                << " is not the code card file (0384, or p0 of version 1.2)"
                << std::endl;

    const auto &dec = resource.data;
    std::cerr << "Decompressed to " << dec.size() << " bytes" << std::endl;

    const bool version12 =
        converter::gameData::version12::find(fileId) != nullptr;
    auto cards = converter::codeCards::parse(
        dec, version12 ? converter::codeCards::consts::VERSION12_CARD_SIZE
                       : converter::codeCards::consts::CARD_SIZE);
    auto json = converter::codeCards::toJson(cards);
    filesystem::writeFile::writeFile(outputPath, json);
    std::cerr << "Wrote " << outputPath << " (" << json.size() << " bytes)"
              << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
