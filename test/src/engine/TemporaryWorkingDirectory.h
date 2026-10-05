#ifndef TEST_SRC_ENGINE_TEMPORARYWORKINGDIRECTORY_H_
#define TEST_SRC_ENGINE_TEMPORARYWORKINGDIRECTORY_H_

#include "../../TemporaryPath.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace openfranko {
namespace test {
namespace src {
namespace engine {

class TemporaryWorkingDirectory {
public:
  explicit TemporaryWorkingDirectory(const std::string &name)
      : m_directory(name), m_previous(std::filesystem::current_path()) {
    std::filesystem::create_directories(m_directory.path());
    std::filesystem::current_path(m_directory.path());
  }

  ~TemporaryWorkingDirectory() { std::filesystem::current_path(m_previous); }

  TemporaryWorkingDirectory(const TemporaryWorkingDirectory &) = delete;
  TemporaryWorkingDirectory &
  operator=(const TemporaryWorkingDirectory &) = delete;

private:
  TemporaryPath m_directory;
  std::filesystem::path m_previous;
};

inline void writeFile(const std::filesystem::path &path,
                      const std::string &contents) {
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream(path, std::ios::binary) << contents;
}

} // namespace engine
} // namespace src
} // namespace test
} // namespace openfranko

#endif // TEST_SRC_ENGINE_TEMPORARYWORKINGDIRECTORY_H_
