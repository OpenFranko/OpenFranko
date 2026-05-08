#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace backwardLZ77 {

class BitReader {
public:
  BitReader(const std::vector<uint8_t> &data, size_t endPos,
            uint32_t initialBuffer, uint32_t xorChecksum);

  uint8_t readRawByte();
  uint32_t getBit();
  uint32_t getBits(int count);
  bool verifyChecksum() const;

private:
  void refill();

  const std::vector<uint8_t> &m_data;
  size_t m_readPos;
  uint32_t m_buffer;
  uint32_t m_checksum;
  uint32_t m_lastBitBeforeRefill = 0;
};

} // namespace backwardLZ77
} // namespace decompressor
} // namespace lib
} // namespace openfranko