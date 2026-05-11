#include "headers.h"
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {

class SPACKScreen {
public:
  SPACKScreen(const std::vector<uint8_t> &data);

  std::vector<uint8_t> getData() const;

private:
  void checkSize();

  void parseHeaders();

  const std::vector<uint8_t> &m_data;
  headers::SPACKHeader m_spackHeader;
  std::vector<uint8_t> m_bitmapData;
  headers::BitmapHeader m_bitmapHeader;
};

} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko