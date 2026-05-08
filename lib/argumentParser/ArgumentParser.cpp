#include "ArgumentParser.h"

namespace openfranko::lib::argumentParser {

ArgumentParser::ArgumentParser(int &argc, char **argv) {
  for (int i = 1; i < argc; ++i)
    this->inputStrings.push_back(std::string(argv[i]));
}

std::optional<std::string>
ArgumentParser::getCmdOption(const std::string &option) const {
  auto itr =
      std::find(this->inputStrings.begin(), this->inputStrings.end(), option);
  if (itr != this->inputStrings.end() && ++itr != this->inputStrings.end()) {
    return *itr;
  }
  return std::nullopt;
}

bool ArgumentParser::cmdOptionExists(const std::string &option) const {
  return std::find(this->inputStrings.begin(), this->inputStrings.end(),
                   option) != this->inputStrings.end();
}

} // namespace openfranko::lib::argumentParser