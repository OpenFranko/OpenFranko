#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/audioExtractor/audioExtractor.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/gameData/gameData.h"
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
    std::cerr << "Usage: " << argv[0]
              << " -i <input_file> [-o <output_dir>] [-m <mode>]" << std::endl;
    std::cerr << "Extracts audio samples from a Franko data file to WAV."
              << std::endl;
    std::cerr << "  -m embedded   treat input as a sprite bank with embedded "
                 "samples (type 0x0000, version 1.2 s files; default for them)"
              << std::endl;
    std::cerr << "  -m standalone treat input as a standalone sample bank "
                 "(type 0x0300, default for other files)"
              << std::endl;
    return 1;
  }

  const std::string inputPath = inputOption.value();
  const auto outputOption = parser.option("-o");
  const auto modeOption = parser.option("-m");

  try {
    const auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;
    const auto resource = converter::fileContainer::unpack(
        std::filesystem::path(inputPath).filename().string(), raw);
    const std::string &fileId = resource.fileId;
    const bool embedded = modeOption.has_value()
                              ? modeOption.value() == "embedded"
                              : resource.resourceType ==
                                    converter::gameData::resourceTypes::SPRITES;
    if (embedded &&
        resource.resourceType != converter::gameData::resourceTypes::SPRITES) {
      std::cerr << "Warning: " << fileId
                << " is not a sprite bank (type 0x0000, or a version 1.2 s "
                   "file)"
                << std::endl;
    }
    if (!embedded &&
        resource.resourceType != converter::gameData::resourceTypes::SAMPLES) {
      std::cerr << "Warning: " << fileId
                << " is not a sample bank (type 0x0300)" << std::endl;
    }

    const auto &decompressed = resource.data;
    std::cerr << "Decompressed to " << decompressed.size() << " bytes"
              << std::endl;

    const std::string outputDir = outputOption.value_or("extracted");
    std::filesystem::create_directories(outputDir);

    const auto samples =
        embedded
            ? converter::audioExtractor::extractEmbeddedSamBank(decompressed,
                                                                fileId)
            : converter::audioExtractor::extractStandaloneSamBank(decompressed,
                                                                  fileId);
    for (const auto &sample : samples) {
      const std::string path = outputDir + "/" + sample.name;
      filesystem::writeFile::writeFile(path, sample.data);
      std::cerr << "Wrote " << path << " (" << sample.data.size() << " bytes)"
                << std::endl;
    }

    if (samples.empty()) {
      std::cerr << "No samples extracted." << std::endl;
    } else {
      std::cerr << "Wrote " << samples.size() << " samples to " << outputDir
                << std::endl;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
