#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/endingCredits/endingCredits.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <iostream>

using namespace openfranko::lib;

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");
  if (!inputOptional.has_value()) {
    std::cerr << "Usage: " << argv[0]
              << " -i <game executable> [-o <credits.json>] [-t <intro.json>]"
              << std::endl;
    std::cerr << "Extracts the ending credits from the compiled Franko game, "
                 "and with -t the intro texts of version 1.2."
              << std::endl;
    return 1;
  }
  const std::string outputPath =
      parser.getCmdOption("-o").value_or("credits.json");

  try {
    const auto executable = filesystem::readFile::readFile(*inputOptional);
    const auto pages = converter::endingCredits::extract(executable);
    std::size_t lines = 0;
    for (const auto &page : pages) {
      lines += page.lines.size();
    }
    std::cerr << "Ending credits: " << pages.size() << " pages, " << lines
              << " lines" << std::endl;
    const auto json = converter::endingCredits::toJson(pages);
    filesystem::writeFile::writeFile(outputPath, json);
    std::cerr << "Wrote " << outputPath << " (" << json.size() << " bytes)"
              << std::endl;
    if (const auto introPath = parser.getCmdOption("-t")) {
      const auto intro = converter::endingCredits::extractIntro(executable);
      const auto introJson = converter::endingCredits::toJson(intro);
      filesystem::writeFile::writeFile(*introPath, introJson);
      std::cerr << "Intro texts: " << intro.size() << " pages -> " << *introPath
                << std::endl;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
