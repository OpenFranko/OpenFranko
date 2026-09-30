#ifndef ENGINE_ASSETS_ARCHIVEFILES_H_
#define ENGINE_ASSETS_ARCHIVEFILES_H_

#include "Files.h"

#include <cstddef>
#include <fstream>
#include <map>
#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

class ArchiveFiles : public Files {
public:
  explicit ArchiveFiles(const std::string &path);

  bool exists(const std::string &path) const override;
  std::vector<std::string> list(const std::string &directory) const override;
  systems::graphics::IndexedBitmap loadBitmap(const std::string &path) override;
  std::vector<uint8_t> read(const std::string &path) override;

private:
  struct Entry {
    std::size_t offset = 0;
    std::size_t size = 0;
  };

  const Entry *find(const std::string &path) const;
  std::vector<uint8_t> contents(const Entry &entry, const std::string &path);
  void readInto(const Entry &entry, uint8_t *target, const std::string &path);

  std::ifstream m_archive;
  std::map<std::string, Entry> m_entries;
  std::vector<uint8_t> m_bitmapFile;
};

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_ARCHIVEFILES_H_
