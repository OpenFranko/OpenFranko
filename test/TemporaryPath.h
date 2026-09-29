#ifndef TEST_TEMPORARYPATH_H_
#define TEST_TEMPORARYPATH_H_

#include <filesystem>
#include <string>

namespace openfranko {
namespace test {

class TemporaryPath {
public:
  explicit TemporaryPath(const std::string &name)
      : m_path(std::filesystem::temp_directory_path() / name) {
    std::filesystem::remove_all(m_path);
  }

  ~TemporaryPath() { std::filesystem::remove_all(m_path); }

  TemporaryPath(const TemporaryPath &) = delete;
  TemporaryPath &operator=(const TemporaryPath &) = delete;

  const std::filesystem::path &path() const { return m_path; }

private:
  std::filesystem::path m_path;
};

} // namespace test
} // namespace openfranko

#endif // TEST_TEMPORARYPATH_H_
