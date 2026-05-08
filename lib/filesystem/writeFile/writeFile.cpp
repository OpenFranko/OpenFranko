#include "writeFile.h"
#include <fstream>

namespace openfranko::lib::filesystem::writeFile {

void writeFile(const std::string &filePath, const std::vector<uint8_t> &data) {
  std::ofstream stream(filePath, std::ios::out | std::ios::binary);
  stream.write(reinterpret_cast<const char *>(data.data()), data.size());
}

} // namespace openfranko::lib::filesystem::writeFile