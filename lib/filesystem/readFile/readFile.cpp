#include "readFile.h"
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace openfranko::lib::filesystem::readFile {

std::vector<uint8_t> readFile(const std::string &filePath) {

  std::ifstream stream(filePath, std::ios::in | std::ios::binary);
  if (!stream) {
    throw std::runtime_error("Cannot open file for reading: " + filePath);
  }

  std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)),
                                std::istreambuf_iterator<char>());

  if (stream.bad()) {
    throw std::runtime_error("Error while reading file: " + filePath);
  }

  return contents;
}

} // namespace openfranko::lib::filesystem::readFile