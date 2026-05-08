#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace lib {
namespace filesystem {
namespace readFile {

std::vector<uint8_t> readFile(const std::string &filePath);

}
} // namespace filesystem
} // namespace lib
} // namespace openfranko