#include "fileContainer.h"
#include "../../helpers/helpers.h"
#include <stdexcept>

namespace openfranko::lib::converter::fileContainer {

static constexpr size_t SUFFIX_SIZE = 8;

FileInfo parseFooter(const std::vector<uint8_t> &rawData) {
  if (rawData.size() < SUFFIX_SIZE) {
    throw std::runtime_error("File too small for game suffix");
  }

  size_t off = rawData.size() - SUFFIX_SIZE;

  FileInfo info;
  info.unpackSize = helpers::readUint32BigEndian(rawData, off);
  info.fileId = helpers::readUint16BigEndian(rawData, off + 4);
  info.resourceType = helpers::readUint16BigEndian(rawData, off + 6);
  return info;
}

} // namespace openfranko::lib::converter::fileContainer
