#include "ArgumentParser.h"

namespace openfranko::lib::argumentParser {

ArgumentParser::ArgumentParser(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    m_inputStrings.push_back(std::string(argv[i]));
  }
}

std::optional<std::string>
ArgumentParser::option(const std::string &name) const {
  auto found = std::find(m_inputStrings.begin(), m_inputStrings.end(), name);
  if (found != m_inputStrings.end() && ++found != m_inputStrings.end()) {
    return *found;
  }
  return std::nullopt;
}

} // namespace openfranko::lib::argumentParser
