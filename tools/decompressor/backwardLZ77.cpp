
#include "../../lib/decompressor/backwardLZ77/backwardLZ77.h"
#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  if (parser.cmdOptionExists("-i")) {

    std::string inputFilePath = parser.getCmdOption("-i").value();

    std::string outputFilePath = inputFilePath + ".dec";

    if (parser.cmdOptionExists("-o")) {
      outputFilePath = parser.getCmdOption("-o").value();
    }

    std::vector<uint8_t> compressedData =
        filesystem::readFile::readFile(inputFilePath);
    std::vector<uint8_t> decompressedData =
        decompressor::backwardLZ77::decompress(compressedData);
    filesystem::writeFile::writeFile(outputFilePath, decompressedData);
  } else {
    std::cerr << "Usage: " << argv[0] << " -i <input_file> -o <output_file>"
              << std::endl;
  }

  return 0;
}