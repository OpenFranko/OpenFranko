#include "writeFile.h"
#include <fstream>
#include <stdexcept>

namespace openfranko::lib::filesystem::writeFile {

void writeFile(const std::string &filePath, const std::vector<uint8_t> &data) {
  std::ofstream stream(filePath, std::ios::out | std::ios::binary);
  if (!stream) {
    throw std::runtime_error("Cannot open file for writing: " + filePath);
  }

  stream.write(reinterpret_cast<const char *>(data.data()),
               static_cast<std::streamsize>(data.size()));
  stream.flush();

  if (!stream) {
    throw std::runtime_error("Error while writing file: " + filePath);
  }
}

} // namespace openfranko::lib::filesystem::writeFile