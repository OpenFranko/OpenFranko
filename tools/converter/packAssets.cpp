#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/packedArchive/packedArchive.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);
  const auto input = parser.option("-i");
  const auto output = parser.option("-o");
  if (!input || !output) {
    std::cerr << "Usage: " << argv[0] << " -i <assets_dir> -o <archive_file>"
              << std::endl;
    std::cerr << "Packs the directory made by frankoExtract into one "
                 "compressed asset archive."
              << std::endl;
    return 1;
  }
  try {
    const std::vector<uint8_t> archive =
        converter::packedArchive::packDirectory(*input, "assets");
    std::ofstream file(*output, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char *>(archive.data()),
               static_cast<std::streamsize>(archive.size()));
    if (!file) {
      std::cerr << "Failed to write " << *output << std::endl;
      return 1;
    }
    std::cerr << "Wrote " << *output << " (" << archive.size() << " bytes)"
              << std::endl;
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << std::endl;
    return 1;
  }
  return 0;
}
