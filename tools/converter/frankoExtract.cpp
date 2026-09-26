#include "../../lib/argumentParser/ArgumentParser.h"
#include "frankoResourceExtractor.h"
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
    std::cerr << "Usage: " << argv[0]
              << " -i <file_or_dir> [-o <output_dir>] [-e <game executable>]"
              << std::endl;
    std::cerr << "Extracts all Franko game data files to standard formats."
              << std::endl;
    std::cerr << "Reads version 1.0 files (0000-03C3) and version 1.2 files "
                 "(m1-m11, p0-p85, s0-s255, t11-t40)."
              << std::endl;
    std::cerr << "  Sprites (0x0000) -> sheet BMP + embedded WAV samples"
              << std::endl;
    std::cerr << "  Icons   (0x0200) -> BMP (screens, tiles, bitmaps)"
              << std::endl;
    std::cerr << "  Levels  (0x0200) -> JSON (level scripts 0385-0387, p1-p3)"
              << std::endl;
    std::cerr
        << "  Codes   (0x0200) -> JSON (copy protection cards in 0384, p0)"
        << std::endl;
    std::cerr << "  Samples (0x0300) -> WAV" << std::endl;
    std::cerr << "  Music   (0x0400) -> S3M (ScreamTracker 3)" << std::endl;
    std::cerr << "  Screen  (0x0201) -> BMP (raw SPACK)" << std::endl;
    std::cerr << "  Game executable (-e, or 'game' in the input directory)"
              << std::endl;
    std::cerr << "          -> JSON (ending credits)" << std::endl;
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
      const std::vector<std::string> files = extractor::dataFiles(inputPath);

      int missing = extractor::validateDirectory(inputPath);
      if (missing > 0) {
        std::cerr << missing << " expected game data file(s) missing."
                  << std::endl;
        return 1;
      }

      std::cerr << "Processing " << files.size() << " data files..."
                << std::endl;
      for (const auto &f : files) {
        errors += extractor::processFile(f, outDir);
      }

      std::cerr << "\nDone. " << files.size() << " files processed, " << errors
                << " errors." << std::endl;
    } else {
      errors = extractor::processFile(inputPath, outDir);
    }

    std::string executable = parser.getCmdOption("-e").value_or("");
    const auto bundled = std::filesystem::path(inputPath) / "game";
    if (executable.empty() && std::filesystem::is_directory(inputPath) &&
        std::filesystem::is_regular_file(bundled)) {
      executable = bundled.string();
    }
    if (executable.empty()) {
      std::cerr << "No game executable given (-e): the ending credits were "
                   "not extracted."
                << std::endl;
    } else {
      errors += extractor::processExecutable(executable, outDir);
    }

    return errors > 0 ? 1 : 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
