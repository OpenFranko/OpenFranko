#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/abkToS3m/abkToS3m.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");

  if (inputOptional.has_value()) {
    std::string inputFilePath = inputOptional.value();

    std::string outputFilePath = inputFilePath;
    if (outputFilePath.size() >= 4 &&
        outputFilePath.substr(outputFilePath.size() - 4) == ".abk") {
      outputFilePath = outputFilePath.substr(0, outputFilePath.size() - 4);
    }
    outputFilePath += ".s3m";

    const auto outputOptional = parser.getCmdOption("-o");
    if (outputOptional.has_value()) {
      outputFilePath = outputOptional.value();
    }

    auto abkData = filesystem::readFile::readFile(inputFilePath);
    auto s3mData = converter::abkToS3m::convert(abkData);
    filesystem::writeFile::writeFile(outputFilePath, s3mData);

    std::cerr << "Converted " << inputFilePath << " -> " << outputFilePath
              << " (" << s3mData.size() << " bytes)" << std::endl;
  } else {
    std::cerr << "Usage: " << argv[0] << " -i <input.abk> [-o <output.s3m>]"
              << std::endl;
  }

  return 0;
}
