#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/endingCredits/endingCredits.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"

#include <iostream>
#include <string>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOption = parser.option("-i");
  if (!inputOption.has_value()) {
    std::cerr << "Usage: " << argv[0]
              << " -i <game_executable> [-o <output_file>] [-t <intro_file>]"
              << std::endl;
    std::cerr << "Extracts the ending credits from the compiled Franko game, "
                 "and with -t the intro texts of version 1.2."
              << std::endl;
    return 1;
  }

  const std::string inputPath = inputOption.value();
  const auto outputOption = parser.option("-o");
  const auto introOption = parser.option("-t");

  try {
    const auto executable = filesystem::readFile::readFile(inputPath);
    std::cerr << "Read " << executable.size() << " bytes" << std::endl;

    const auto pages = converter::endingCredits::extract(executable);
    std::size_t lines = 0;
    for (const auto &page : pages) {
      lines += page.lines.size();
    }
    std::cerr << "Ending credits: " << pages.size() << " pages, " << lines
              << " lines" << std::endl;
    const auto json = converter::endingCredits::toJson(pages);
    const std::string outputPath = outputOption.value_or("credits.json");
    filesystem::writeFile::writeFile(outputPath, json);
    std::cerr << "Wrote " << outputPath << " (" << json.size() << " bytes)"
              << std::endl;

    if (introOption.has_value()) {
      const auto intro = converter::endingCredits::extractIntro(executable);
      std::cerr << "Intro texts: " << intro.size() << " pages" << std::endl;
      const auto introJson = converter::endingCredits::toJson(intro);
      const std::string introPath = introOption.value();
      filesystem::writeFile::writeFile(introPath, introJson);
      std::cerr << "Wrote " << introPath << " (" << introJson.size()
                << " bytes)" << std::endl;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
