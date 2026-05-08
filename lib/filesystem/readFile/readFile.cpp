#include "readFile.h"
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace openfranko::lib::filesystem::readFile {

std::vector<uint8_t> readFile(const std::string &filePath) {

  std::ifstream stream(filePath, std::ios::in | std::ios::binary);
  std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)),
                                std::istreambuf_iterator<char>());

  return contents;
}

} // namespace openfranko::lib::filesystem::readFile