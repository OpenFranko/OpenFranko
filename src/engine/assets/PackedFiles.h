#ifndef ENGINE_ASSETS_PACKEDFILES_H_
#define ENGINE_ASSETS_PACKEDFILES_H_

#include "Files.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

class PackedFiles : public Files {
public:
  PackedFiles(const uint8_t *data, std::size_t size);

  bool exists(const std::string &path) const override;
  std::vector<std::string> list(const std::string &directory) const override;
  std::unique_ptr<Listing> walk(const std::string &directory) const override;
  systems::graphics::IndexedBitmap loadBitmap(const std::string &path) override;
  std::vector<uint8_t> read(const std::string &path) override;

  std::size_t entries() const;

private:
  class Walk;

  struct Entry {
    const char *name = nullptr;
    const uint8_t *data = nullptr;
    std::size_t storedSize = 0;
    std::size_t size = 0;
    uint32_t flags = 0;
  };

  Entry entry(std::size_t index) const;
  std::size_t lowerBound(const std::string &name) const;
  std::size_t indexOf(std::string_view name) const;
  const char *nameAt(std::size_t index) const;
  Entry require(const std::string &path) const;

  const uint8_t *m_data;
  std::size_t m_size;
  std::size_t m_count;
  mutable std::size_t m_next = 0;
};

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_PACKEDFILES_H_
