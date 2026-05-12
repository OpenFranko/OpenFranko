#include "ByteReader.h"

namespace openfranko::lib::converter::amosCompact::detail {

ByteReader::ByteReader(const std::vector<uint8_t> &data, size_t offset)
    : m_data(data), m_offset(offset) {}

int ByteReader::read() {
  if (m_offset >= m_data.size()) {
    return 0;
  }
  return m_data[m_offset++];
}

} // namespace openfranko::lib::converter::amosCompact::detail