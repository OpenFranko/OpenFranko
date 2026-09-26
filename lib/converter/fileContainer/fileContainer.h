#ifndef FILECONTAINER_H_
#define FILECONTAINER_H_

#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace fileContainer {

struct FileInfo {
  uint32_t unpackSize;
  uint16_t fileId;
  uint16_t resourceType;
  uint8_t bankType;
  bool compressed;
};

struct Resource {
  std::string fileId;
  uint16_t resourceType = 0;
  std::vector<uint8_t> data;
};

FileInfo parseFooter(const std::vector<uint8_t> &rawData);

std::string fileIdToHex(uint16_t fileId);

Resource unpack(const std::string &fileName,
                const std::vector<uint8_t> &rawData);

std::vector<uint8_t> unsquashVersion12(const std::string &fileName,
                                       const std::vector<uint8_t> &rawData);

} // namespace fileContainer
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // FILECONTAINER_H_
