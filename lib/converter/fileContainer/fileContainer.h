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
  uint32_t unpackSize = 0;
  uint16_t fileId = 0;
  uint16_t resourceType = 0;
  uint8_t bankType = 0;
  bool compressed = false;
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
