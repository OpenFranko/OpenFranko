#ifndef BMPWRITER_H_
#define BMPWRITER_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace bmpWriter {

std::vector<uint8_t> pixelsToBmp(uint32_t width, uint32_t height,
                                 const uint8_t *pixels,
                                 const uint16_t *palette, int numberOfColors);

} // namespace bmpWriter
} // namespace lib
} // namespace openfranko

#endif // BMPWRITER_H_
