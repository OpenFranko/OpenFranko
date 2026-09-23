#include "frankoResourceExtractor.h"
#include "../../lib/argumentParser/ArgumentParser.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace openfranko::lib;
namespace extractor = openfranko::tools::converter::frankoResourceExtractor;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");
  if (!inputOptional.has_value()) {
    std::cerr << "Usage: " << argv[0] << " -i <file_or_dir> [-o <output_dir>]"
              << std::endl;
    std::cerr << "Extracts all Franko game data files to standard formats."
              << std::endl;
    std::cerr << "  Sprites (0x0000) -> sheet BMP + embedded WAV samples"
              << std::endl;
    std::cerr << "  Icons   (0x0200) -> BMP (screens, tiles, bitmaps)"
              << std::endl;
    std::cerr << "  Levels  (0x0200) -> JSON (level scripts 0385-0387)"
              << std::endl;
    std::cerr << "  Samples (0x0300) -> WAV" << std::endl;
    std::cerr << "  Music   (0x0400) -> S3M (ScreamTracker 3)" << std::endl;
    std::cerr << "  Screen  (0x0201) -> BMP (raw SPACK)" << std::endl;
    return 1;
  }

  std::string inputPath = inputOptional.value();
  std::string outDir = "extracted";
  const auto outputOptional = parser.getCmdOption("-o");
  if (outputOptional.has_value()) {
    outDir = outputOptional.value();
  }

  try {
    std::filesystem::create_directories(outDir);

    int errors = 0;

    if (std::filesystem::is_directory(inputPath)) {
      std::vector<std::string> files;
      for (const auto &entry : std::filesystem::directory_iterator(inputPath)) {
        if (entry.is_regular_file()) {
          std::string name = entry.path().filename().string();
          if (name.size() == 4) {
            bool isHex = true;
            for (char c : name) {
              if (!std::isxdigit(static_cast<unsigned char>(c))) {
                isHex = false;
                break;
              }
            }
            if (isHex) {
              files.push_back(entry.path().string());
            }
          }
        }
      }
      std::sort(files.begin(), files.end());

      int missing = extractor::validateDirectory(inputPath);
      if (missing > 0) {
        std::cerr << missing << " expected game data file(s) missing."
                  << std::endl;
        return 1;
      }

      extractor::SeenSampleBanks seenSamBanks;
      std::cerr << "Processing " << files.size() << " data files..."
                << std::endl;
      for (const auto &f : files) {
        errors += extractor::processFile(f, outDir, seenSamBanks);
      }

      std::cerr << "\nDone. " << files.size() << " files processed, " << errors
                << " errors." << std::endl;
    } else {
      extractor::SeenSampleBanks seenSamBanks;
      errors = extractor::processFile(inputPath, outDir, seenSamBanks);
    }

    return errors > 0 ? 1 : 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
