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

  std::optional<std::string> getCmdOption(const std::string &option) const;

private:
  std::vector<std::string> inputStrings;
};

} // namespace argumentParser
} // namespace lib
} // namespace openfranko