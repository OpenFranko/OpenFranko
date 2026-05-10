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
  struct SPACKHeader {
    uint16_t screenWidth;
    uint16_t screenHeight;
    uint16_t windowX;
    uint16_t windowY;
    uint16_t windowWidth;
    uint16_t windowHeight;
    uint16_t viewX;
    uint16_t viewY;
    uint16_t displayModeFlags;
    uint16_t numberOfColors;
    uint16_t numberOfBitplanes;
    std::vector<uint16_t> amigaPalette;
  };

  void parseHeader();

  const std::vector<uint8_t> &m_data;
  SPACKHeader m_header;
};

} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko