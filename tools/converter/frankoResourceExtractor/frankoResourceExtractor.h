#ifndef FRANKORESOURCEEXTRACTOR_H_
#define FRANKORESOURCEEXTRACTOR_H_

#include <string>
#include <vector>

namespace openfranko {
namespace tools {
namespace converter {
namespace frankoResourceExtractor {

std::vector<std::string> dataFiles(const std::string &inputDir);

int validateDirectory(const std::string &inputDir);

int processFile(const std::string &inputPath, const std::string &outputDir);

int processExecutable(const std::string &inputPath,
                      const std::string &outputDir);

} // namespace frankoResourceExtractor
} // namespace converter
} // namespace tools
} // namespace openfranko

#endif // FRANKORESOURCEEXTRACTOR_H_
