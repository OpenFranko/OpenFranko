#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/audioExtractor/audioExtractor.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <filesystem>
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");
  if (!inputOptional.has_value()) {
    std::cerr << "Usage: " << argv[0] << " -i <input_file> [-o <output.abk>]"
              << std::endl;
    std::cerr
        << "Extracts a Franko music bank (type 0x0400, or a version 1.2 m "
           "file) to ABK format."
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
    std::string outputPath = outputOptional.value_or(fileId + ".abk");

    const auto &dec = resource.data;
    std::cerr << "Decompressed to " << dec.size() << " bytes" << std::endl;

    auto abk = converter::audioExtractor::wrapMusicBank(dec, fileId);
    filesystem::writeFile::writeFile(outputPath, abk.data);
    std::cerr << "Wrote " << outputPath << " (" << abk.data.size()
              << " bytes)" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
