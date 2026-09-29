#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/abkToS3m/abkToS3m.h"
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
    std::cerr << "Converts an AMOS music bank (ABK) to a ScreamTracker 3 "
                 "module (S3M)."
              << std::endl;
    return 1;
  }

  const std::string inputPath = inputOption.value();
  const auto outputOption = parser.option("-o");

  try {
    const auto abk = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << abk.size() << " bytes" << std::endl;

    const auto s3m = converter::abkToS3m::convert(abk);
    const std::string outputPath = outputOption.value_or(
        std::filesystem::path(inputPath).stem().string() + ".s3m");
    filesystem::writeFile::writeFile(outputPath, s3m);
    std::cerr << "Wrote " << outputPath << " (" << s3m.size() << " bytes)"
              << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
