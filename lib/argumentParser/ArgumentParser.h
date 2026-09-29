#ifndef ARGUMENTPARSER_H_
#define ARGUMENTPARSER_H_

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace openfranko {
namespace lib {
namespace argumentParser {

class ArgumentParser {
public:
  ArgumentParser(int argc, char **argv);

  std::optional<std::string> option(const std::string &name) const;

private:
  std::vector<std::string> m_inputStrings;
};

} // namespace argumentParser
} // namespace lib
} // namespace openfranko

#endif // ARGUMENTPARSER_H_
