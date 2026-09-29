#ifndef BINARY_H_
#define BINARY_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace binary {

class BigEndianReader {
public:
  explicit BigEndianReader(const std::vector<uint8_t> &data);

  uint32_t readUint32(size_t pos) const;
  uint16_t readUint16(size_t pos) const;
  int32_t readInt32(size_t pos) const;
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

void pushBigEndian16(std::vector<uint8_t> &buf, uint16_t value);
void pushBigEndian32(std::vector<uint8_t> &buf, uint32_t value);
void pushLittleEndian16(std::vector<uint8_t> &buf, uint16_t value);
void pushLittleEndian32(std::vector<uint8_t> &buf, uint32_t value);
void writeLittleEndian16(std::vector<uint8_t> &buf, size_t pos, uint16_t value);
void writeLittleEndian32(std::vector<uint8_t> &buf, size_t pos, uint32_t value);
void padTo16(std::vector<uint8_t> &buf);

} // namespace binary
} // namespace lib
} // namespace openfranko

#endif // BINARY_H_
