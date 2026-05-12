#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/bitmapExtractor/bitmapExtractor.h"
#include "../../lib/decompressor/backwardLZ77/backwardLZ77.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <filesystem>
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");
  if (!inputOptional.has_value()) {
    std::cerr << "Usage: " << argv[0] << " -i <input_file> [-o <output_dir>]"
              << std::endl;
    std::cerr << "Extracts bitmaps from a Franko icon/bitmap file (type "
                 "0x0200)."
              << std::endl;
    std::cerr << "Handles screens, tiles, and multi-bitmap files." << std::endl;
    return 1;
  }

  std::string inputPath = inputOptional.value();
  std::string fileId =
      std::filesystem::path(inputPath).filename().string();

  std::string outDir = ".";
  const auto outputOptional = parser.getCmdOption("-o");
  if (outputOptional.has_value())
    outDir = outputOptional.value();

  try {
    auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;

    auto dec = decompressor::backwardLZ77::decompress(raw);
    std::cerr << "Decompressed to " << dec.size() << " bytes" << std::endl;

    std::filesystem::create_directories(outDir);

    auto bitmaps = converter::bitmapExtractor::extract(dec, fileId);
    for (const auto &bm : bitmaps) {
      std::string path = outDir + "/" + bm.name + ".bmp";
      filesystem::writeFile::writeFile(path, bm.bmpData);
      std::cerr << "  -> " << path << " (" << bm.bmpData.size() << " bytes)"
                << std::endl;
    }

    if (bitmaps.empty())
      std::cerr << "No bitmaps extracted." << std::endl;
    else
      std::cerr << "Wrote " << bitmaps.size() << " bitmaps to " << outDir
                << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
