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
  helpers::BigEndianReader reader(rawData);

  FileInfo info;
  info.unpackSize = reader.readUint32(off);
  info.fileId = reader.readUint16(off + 4);
  info.resourceType = reader.readUint16(off + 6);
  return info;
}

} // namespace openfranko::lib::converter::fileContainer
