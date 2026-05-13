#ifndef ABKTOS3M_H_
#define ABKTOS3M_H_

#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace abkToS3m {

std::vector<uint8_t> convert(const std::vector<uint8_t> &abkData);

} // namespace abkToS3m
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // ABKTOS3M_H_
