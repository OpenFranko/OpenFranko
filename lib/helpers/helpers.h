#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace helpers {

uint32_t readUint32BigEndian(const std::vector<uint8_t> &data, size_t pos);
uint16_t readUint16BigEndian(const std::vector<uint8_t> &data, size_t pos);
int16_t readInt16BigEndian(const std::vector<uint8_t> &data, size_t pos);

uint32_t readUint32LittleEndian(const std::vector<uint8_t> &data, size_t pos);
uint16_t readUint16LittleEndian(const std::vector<uint8_t> &data, size_t pos);

void pushBigEndian16(std::vector<uint8_t> &buf, uint16_t v);
void pushBigEndian32(std::vector<uint8_t> &buf, uint32_t v);
void pushLittleEndian16(std::vector<uint8_t> &buf, uint16_t v);
void pushLittleEndian32(std::vector<uint8_t> &buf, uint32_t v);
void padTo16(std::vector<uint8_t> &buf);

} // namespace helpers
} // namespace lib
} // namespace openfranko