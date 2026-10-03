#ifndef ENGINE_STREET_CORE_HIGHSCORESTORAGE_H_
#define ENGINE_STREET_CORE_HIGHSCORESTORAGE_H_

#include "HighScoreTable.h"

#include <array>
#include <cstdint>
#include <optional>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace core {

inline constexpr std::size_t STORAGE_WORDS = 63;

using StorageWords = std::array<uint16_t, STORAGE_WORDS>;

StorageWords packHighScores(const HighScoreTable &table);
std::optional<HighScoreTable> unpackHighScores(const StorageWords &words);

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CORE_HIGHSCORESTORAGE_H_
