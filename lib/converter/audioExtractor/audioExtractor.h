#ifndef AUDIOEXTRACTOR_H_
#define AUDIOEXTRACTOR_H_

#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace audioExtractor {

struct ExtractedAudio {
  std::string name;
  std::vector<uint8_t> data;
};

std::vector<ExtractedAudio>
extractStandaloneSamBank(const std::vector<uint8_t> &data,
                         const std::string &fileId);

std::vector<ExtractedAudio>
extractEmbeddedSamBank(const std::vector<uint8_t> &data,
                       const std::string &fileId);

ExtractedAudio wrapMusicBank(const std::vector<uint8_t> &data,
                             const std::string &fileId);

} // namespace audioExtractor
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // AUDIOEXTRACTOR_H_
