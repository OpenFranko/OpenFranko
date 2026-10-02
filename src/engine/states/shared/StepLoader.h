#ifndef ENGINE_STATES_SHARED_STEPLOADER_H_
#define ENGINE_STATES_SHARED_STEPLOADER_H_

#include "../../../systems/audio/Speaker.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../assets/Files.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

class StepLoader {
public:
  using Step = std::function<bool()>;

  std::size_t add(Step step);
  void step(int count);
  void finish(std::size_t task);
  bool isDone(std::size_t task) const;

private:
  bool runNext();

  std::vector<Step> m_tasks;
  std::size_t m_next = 0;
};

StepLoader::Step bitmapStep(assets::Files &files, std::string path,
                            systems::graphics::IndexedBitmap &bitmap);
StepLoader::Step musicStep(assets::Files &files,
                           systems::audio::Speaker &speaker, std::string path);

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SHARED_STEPLOADER_H_
