#ifndef PACKEDARCHIVE_H_
#define PACKEDARCHIVE_H_

#include "../../../src/systems/graphics/Bitmap.h"

#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace packedArchive {

class ArchiveWriter {
public:
  void addFile(const std::string &name, const std::vector<uint8_t> &data);
  void addBitmap(const std::string &name,
                 const src::systems::graphics::IndexedBitmap &bitmap);
  std::vector<uint8_t> finish() const;

private:
  struct Entry {
    std::string name;
    std::vector<uint8_t> stored;
    uint32_t size = 0;
    uint32_t flags = 0;
  };

  std::vector<Entry> m_entries;
};

std::vector<uint8_t> packDirectory(const std::string &directory,
                                   const std::string &prefix);

} // namespace packedArchive
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // PACKEDARCHIVE_H_
