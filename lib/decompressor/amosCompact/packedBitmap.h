#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {

class PackedBitmap {
public:
  PackedBitmap(const std::vector<uint8_t> &data);

  std::vector<uint8_t> getData() const;

private:
  struct BitmapHeader {
    int16_t xOffset;
    int16_t yOffset;
    uint16_t bytesWidth;
    uint16_t rowsHeight;
    uint16_t tileHeight;
    uint16_t numberOfBitplanes;
    uint32_t offsetToByteTable;
    uint32_t offsetToPointerTable;
  };

  void parseHeader();

  const std::vector<uint8_t> &m_data;
  BitmapHeader m_header;
};

} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko