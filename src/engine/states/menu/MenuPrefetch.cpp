#include "MenuPrefetch.h"

#include "MenuState.h"

#include <exception>
#include <utility>

namespace openfranko::src::engine::states::menu {
namespace {

constexpr int STEPS_PER_FRAME = 2;

} // namespace

MenuPrefetch::MenuPrefetch(assets::PrefetchingFiles &files,
                           systems::audio::AudioSystem &audio)
    : m_files(files), m_audio(audio) {}

void MenuPrefetch::start(GameVersion version) {
  stop();
  m_version = version;
  m_running = true;
  m_queued = 0;
  if (version == GameVersion::V12) {
    m_tunePath = MenuState::tunePath(version);
    m_tuneRead = m_files.beginRead(m_tunePath);
  }
}

void MenuPrefetch::pause() { m_running = false; }

void MenuPrefetch::stop() {
  m_running = false;
  m_files.drop();
  m_tuneRead.reset();
  m_tune.clear();
  m_audio.dropPreparedMusic();
}

void MenuPrefetch::step() {
  for (int done = 0; done < STEPS_PER_FRAME && advance(); ++done) {
  }
}

bool MenuPrefetch::advance() {
  if (!m_running) {
    return false;
  }
  if (m_files.step()) {
    return true;
  }
  if (m_tuneRead) {
    try {
      if (m_tuneRead->step(m_tune)) {
        m_audio.prepareMusic(m_tunePath, std::move(m_tune));
        m_tuneRead.reset();
      }
    } catch (const std::exception &) {
      m_tuneRead.reset();
    }
    return true;
  }
  if (m_audio.stepPreparation()) {
    return true;
  }
  if (m_queued == 0) {
    m_files.prefetchMore(MenuState::menuPaths(m_version));
  } else if (m_queued == 1) {
    m_files.prefetchMore(MenuState::attractPaths(m_version));
  } else {
    m_running = false;
  }
  ++m_queued;
  return m_running;
}

} // namespace openfranko::src::engine::states::menu
