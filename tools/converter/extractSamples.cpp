#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/audioExtractor/audioExtractor.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/gameData/gameData.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <filesystem>
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");
  if (!inputOptional.has_value()) {
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

  std::string inputPath = inputOptional.value();

  std::string outDir = ".";
  const auto outputOptional = parser.getCmdOption("-o");
  if (outputOptional.has_value())
    outDir = outputOptional.value();

  const auto modeOptional = parser.getCmdOption("-m");

  try {
    auto raw = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << raw.size() << " bytes" << std::endl;
    auto resource = converter::fileContainer::unpack(
        std::filesystem::path(inputPath).filename().string(), raw);
    const std::string &fileId = resource.fileId;

    const auto &dec = resource.data;
    std::cerr << "Decompressed to " << dec.size() << " bytes" << std::endl;

    const bool embedded = modeOptional.has_value()
                              ? modeOptional.value() == "embedded"
                              : resource.resourceType ==
                                    converter::gameData::resourceTypes::SPRITES;

    std::filesystem::create_directories(outDir);

    std::vector<converter::audioExtractor::ExtractedAudio> samples;
    if (embedded) {
      samples = converter::audioExtractor::extractEmbeddedSamBank(dec, fileId);
    } else {
      samples = converter::audioExtractor::extractStandaloneSamBank(dec, fileId);
    }

    for (const auto &s : samples) {
      std::string path = outDir + "/" + s.name;
      filesystem::writeFile::writeFile(path, s.data);
      std::cerr << "  -> " << path << " (" << s.data.size() << " bytes)"
                << std::endl;
    }

    if (samples.empty()) {
      std::cerr << "(no samples found)" << std::endl;
    } else {
      std::cerr << "Wrote " << samples.size() << " samples to " << outDir
                << std::endl;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
