#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/amosCompact/amosCompact.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");
  if (!inputOptional.has_value()) {
    std::cerr << "Usage: " << argv[0] << " -i <input_file> [-o <output.bmp>]"
              << std::endl;
    std::cerr
        << "Converts a Franko screen package (type 0x0201) to BMP."
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
    std::string outputPath = outputOptional.value_or(fileId + ".bmp");

    auto bmp = converter::amosCompact::decompress(raw);
    filesystem::writeFile::writeFile(outputPath, bmp);
    std::cerr << "Wrote " << outputPath << " (" << bmp.size() << " bytes)"
              << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
