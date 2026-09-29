#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/bitmapExtractor/bitmapExtractor.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <filesystem>
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOption = parser.option("-i");
  if (!inputOption.has_value()) {
    std::cerr << "Usage: " << argv[0] << " -i <input_file> [-o <output_dir>]"
              << std::endl;
    std::cerr << "Extracts bitmaps from a Franko icon/bitmap file (type "
                 "0x0200, or a version 1.2 p or t file)."
              << std::endl;
    std::cerr << "Handles screens, tiles, and multi-bitmap files." << std::endl;
    return 1;
  }

  std::string inputPath = inputOption.value();

  std::string outDir = ".";
  const auto outputOption = parser.option("-o");
  if (outputOption.has_value())
    outDir = outputOption.value();

  try {
    auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;
    auto resource = converter::fileContainer::unpack(
        std::filesystem::path(inputPath).filename().string(), raw);
    const std::string &fileId = resource.fileId;

    const auto &decompressed = resource.data;
    std::cerr << "Decompressed to " << decompressed.size() << " bytes"
              << std::endl;

    std::filesystem::create_directories(outDir);

    auto bitmaps = converter::bitmapExtractor::extract(decompressed, fileId);
    size_t written = 0;
    for (const auto &bitmap : bitmaps) {
      if (!bitmap.error.empty()) {
        std::cerr << "Skipped " << bitmap.name << ": " << bitmap.error
                  << std::endl;
        continue;
      }
      std::string path = outDir + "/" + bitmap.name + ".bmp";
      filesystem::writeFile::writeFile(path, bitmap.bmpData);
      std::cerr << "  -> " << path << " (" << bitmap.bmpData.size() << " bytes)"
                << std::endl;
      written++;
    }

    if (written == 0)
      std::cerr << "No bitmaps extracted." << std::endl;
    else
      std::cerr << "Wrote " << written << " bitmaps to " << outDir << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
