#ifndef TEST_LIB_CONVERTER_BUILDPACKEDBITMAP_H_
#define TEST_LIB_CONVERTER_BUILDPACKEDBITMAP_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace test {
namespace lib {
namespace converter {

inline std::vector<uint8_t>
buildPackedBitmap(uint16_t tx, uint16_t ty, uint16_t tcar, uint16_t nplan,
                  const std::vector<uint8_t> &dataStream1,
                  const std::vector<uint8_t> &dataStream2,
                  const std::vector<uint8_t> &pointerBitstream) {
  uint32_t datas2Off = 24 + static_cast<uint32_t>(dataStream1.size());
  uint32_t pointOff = datas2Off + static_cast<uint32_t>(dataStream2.size());

  std::vector<uint8_t> data = {
      0x06,
      0x07,
      0x19,
      0x63,
      0x00,
      0x00,
      0x00,
      0x00,
      static_cast<uint8_t>(tx >> 8),
      static_cast<uint8_t>(tx & 0xFF),
      static_cast<uint8_t>(ty >> 8),
      static_cast<uint8_t>(ty & 0xFF),
      static_cast<uint8_t>(tcar >> 8),
      static_cast<uint8_t>(tcar & 0xFF),
      static_cast<uint8_t>(nplan >> 8),
      static_cast<uint8_t>(nplan & 0xFF),
      static_cast<uint8_t>(datas2Off >> 24),
      static_cast<uint8_t>(datas2Off >> 16),
      static_cast<uint8_t>(datas2Off >> 8),
      static_cast<uint8_t>(datas2Off & 0xFF),
      static_cast<uint8_t>(pointOff >> 24),
      static_cast<uint8_t>(pointOff >> 16),
      static_cast<uint8_t>(pointOff >> 8),
      static_cast<uint8_t>(pointOff & 0xFF),
  };

  data.insert(data.end(), dataStream1.begin(), dataStream1.end());
  data.insert(data.end(), dataStream2.begin(), dataStream2.end());
  data.insert(data.end(), pointerBitstream.begin(), pointerBitstream.end());

  return data;
}

} // namespace converter
} // namespace lib
} // namespace test
} // namespace openfranko

#endif // TEST_LIB_CONVERTER_BUILDPACKEDBITMAP_H_
