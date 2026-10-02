#include "Files.h"

#include <utility>

namespace openfranko::src::engine::assets {
namespace {

class PathListing : public Files::Listing {
public:
  explicit PathListing(std::vector<std::string> paths)
      : m_paths(std::move(paths)) {}

  bool next(std::string_view &name) override {
    if (m_next >= m_paths.size()) {
      return false;
    }
    name = m_paths[m_next++];
    const std::size_t slash = name.find_last_of("/\\");
    if (slash != std::string_view::npos) {
      name.remove_prefix(slash + 1);
    }
    return true;
  }

private:
  std::vector<std::string> m_paths;
  std::size_t m_next = 0;
};

} // namespace

std::unique_ptr<Files::Listing>
Files::walk(const std::string &directory) const {
  return std::make_unique<PathListing>(list(directory));
}

} // namespace openfranko::src::engine::assets
