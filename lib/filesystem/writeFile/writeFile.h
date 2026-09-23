#ifndef WRITEFILE_H_
#define WRITEFILE_H_

#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace lib {
namespace filesystem {
namespace writeFile {

void writeFile(const std::string &filePath, const std::vector<uint8_t> &data);

}
} // namespace filesystem
} // namespace lib
} // namespace openfranko

#endif // WRITEFILE_H_
