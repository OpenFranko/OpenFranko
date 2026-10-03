#include "StepLoader.h"

#include <exception>
#include <memory>
#include <utility>

namespace openfranko::src::engine::states::shared {

std::size_t StepLoader::add(Step step) {
  m_tasks.push_back(std::move(step));
  return m_tasks.size() - 1;
}

void StepLoader::step(int count) {
  for (int done = 0; done < count && m_next < m_tasks.size(); ++done) {
    runNext();
  }
}

void StepLoader::finish(std::size_t task) {
  while (m_next <= task && m_next < m_tasks.size()) {
    runNext();
  }
}

bool StepLoader::isDone(std::size_t task) const { return task < m_next; }

bool StepLoader::runNext() {
  if (!m_tasks[m_next]()) {
    return false;
  }
  m_tasks[m_next] = nullptr;
  ++m_next;
  return true;
}

StepLoader::Step bitmapStep(assets::Files &files, std::string path,
                            systems::graphics::IndexedBitmap &bitmap) {
  return [&files, path = std::move(path), &bitmap,
          load = std::shared_ptr<assets::Files::BitmapLoad>()]() mutable {
    if (!load) {
      load = files.beginBitmap(path);
    }
    return load->step(bitmap);
  };
}

StepLoader::Step musicStep(assets::Files &files,
                           systems::audio::Speaker &speaker, std::string path) {
  struct Load {
    std::unique_ptr<assets::Files::FileLoad> read;
    std::vector<uint8_t> data;
    std::unique_ptr<systems::audio::Speaker::MusicLoad> music;
  };
  return [&files, &speaker, path = std::move(path),
          load = std::make_shared<Load>()]() {
    if (load->music) {
      return load->music->step();
    }
    try {
      if (!load->read) {
        load->read = files.beginRead(path);
      }
      if (!load->read->step(load->data)) {
        return false;
      }
    } catch (const std::exception &) {
      speaker.loadMusic(path);
      return true;
    }
    load->music = speaker.beginMusic(path, std::move(load->data), 0);
    return false;
  };
}

} // namespace openfranko::src::engine::states::shared
