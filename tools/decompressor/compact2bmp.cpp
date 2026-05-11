#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/decompressor/amosCompact/amosCompact.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");

  if (inputOptional.has_value()) {

    std::string inputFilePath = inputOptional.value();

    std::string outputFilePath = inputFilePath + ".bmp";

    const auto outputOptional = parser.getCmdOption("-o");

    if (outputOptional.has_value()) {
      outputFilePath = outputOptional.value();
    }

    std::vector<uint8_t> compressedData =
        filesystem::readFile::readFile(inputFilePath);
    std::vector<uint8_t> decompressedData =
        decompressor::amosCompact::decompress(compressedData);
    filesystem::writeFile::writeFile(outputFilePath, decompressedData);
  } else {
    std::cerr << "Usage: " << argv[0] << " -i <input_file> -o <output_file>"
              << std::endl;
  }

  return 0;
}