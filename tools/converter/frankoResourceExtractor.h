#ifndef FRANKORESOURCEEXTRACTOR_H_
#define FRANKORESOURCEEXTRACTOR_H_

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace openfranko {
namespace tools {
namespace converter {
namespace frankoResourceExtractor {

using SeenSampleBanks = std::set<std::vector<uint8_t>>;

int validateDirectory(const std::string &dirPath);

int processFile(const std::string &inputPath, const std::string &outDir,
                SeenSampleBanks &seenSamBanks);

} // namespace frankoResourceExtractor
} // namespace converter
} // namespace tools
} // namespace openfranko

#endif // FRANKORESOURCEEXTRACTOR_H_
