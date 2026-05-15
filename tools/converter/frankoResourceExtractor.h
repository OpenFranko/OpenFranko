#ifndef FRANKORESOURCEEXTRACTOR_H_
#define FRANKORESOURCEEXTRACTOR_H_

#include <cstddef>
#include <set>
#include <string>

namespace openfranko {
namespace tools {
namespace converter {
namespace frankoResourceExtractor {

int validateDirectory(const std::string &dirPath);

int processFile(const std::string &inputPath, const std::string &outDir,
                std::set<size_t> &seenSamBanks);

} // namespace frankoResourceExtractor
} // namespace converter
} // namespace tools
} // namespace openfranko

#endif // FRANKORESOURCEEXTRACTOR_H_
