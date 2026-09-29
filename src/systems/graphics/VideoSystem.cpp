#include "graphics/VideoSystem.h"

namespace openfranko::src::systems::graphics {

void VideoSystem::show(const Display &display) {
  m_shown = display;
  m_frameChanged = true;
}

void VideoSystem::clear() {
  m_shown = Display{};
  m_frameChanged = true;
}

void VideoSystem::sync() {
  present();
  waitVbl();
}

void VideoSystem::setNtsc(bool enabled) { m_ntsc = enabled; }

bool VideoSystem::isNtsc() const { return m_ntsc; }

int VideoSystem::refreshRate() const { return m_ntsc ? NTSC_HERTZ : PAL_HERTZ; }

} // namespace openfranko::src::systems::graphics
