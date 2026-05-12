#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/amosCompact/amosCompact.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <filesystem>
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
  std::string fileId =
      std::filesystem::path(inputPath).filename().string();

  std::string outputPath = fileId + ".bmp";
  const auto outputOptional = parser.getCmdOption("-o");
  if (outputOptional.has_value())
    outputPath = outputOptional.value();

  try {
    auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;

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
