#include "fileContainer.h"
#include "../../helpers/helpers.h"
#include <cstdio>
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
  info.bankType = static_cast<uint8_t>(info.resourceType >> 8);
  info.compressed = (info.resourceType & 0xFF) == 0;
  return info;
}

std::string fileIdToHex(uint16_t fileId) {
  char buf[5];
  snprintf(buf, sizeof(buf), "%04X", fileId);
  return buf;
}

} // namespace openfranko::lib::converter::fileContainer
