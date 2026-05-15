#ifndef HELPERS_H_
#define HELPERS_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace helpers {

class BigEndianReader {
public:
  explicit BigEndianReader(const std::vector<uint8_t> &data);

  uint32_t readUint32(size_t pos) const;
  uint16_t readUint16(size_t pos) const;
  int16_t readInt16(size_t pos) const;

private:
  void require(size_t pos, size_t count) const;

  const std::vector<uint8_t> &m_data;
};

class LittleEndianReader {
public:
  explicit LittleEndianReader(const std::vector<uint8_t> &data);

  uint32_t readUint32(size_t pos) const;
  uint16_t readUint16(size_t pos) const;

private:
  void require(size_t pos, size_t count) const;

  const std::vector<uint8_t> &m_data;
};

void pushBigEndian16(std::vector<uint8_t> &buf, uint16_t v);
void pushBigEndian32(std::vector<uint8_t> &buf, uint32_t v);
void pushLittleEndian16(std::vector<uint8_t> &buf, uint16_t v);
void pushLittleEndian32(std::vector<uint8_t> &buf, uint32_t v);
void padTo16(std::vector<uint8_t> &buf);

} // namespace helpers
} // namespace lib
} // namespace openfranko

#endif // HELPERS_H_
