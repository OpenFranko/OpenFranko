
#include "../../lib/decompressor/backwardLZ77/backwardLZ77.h"
#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");

  if (!inputOptional.has_value()) {
    std::cerr << "Usage: " << argv[0] << " -i <input_file> -o <output_file>"
              << std::endl;
    return 1;
  }

  std::string inputFilePath = inputOptional.value();

  std::string outputFilePath = inputFilePath + ".dec";

  const auto outputOptional = parser.getCmdOption("-o");

  if (outputOptional.has_value()) {
    outputFilePath = outputOptional.value();
  }

  try {
    std::vector<uint8_t> compressedData =
        filesystem::readFile::readFile(inputFilePath);
    std::vector<uint8_t> decompressedData =
        decompressor::backwardLZ77::decompress(compressedData);
    filesystem::writeFile::writeFile(outputFilePath, decompressedData);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}