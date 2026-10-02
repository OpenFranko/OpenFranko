#include "graphics/VideoSystem.h"

#include "jaguar/Blitter.h"
#include "jaguar/DebugOverlay.h"
#include "jaguar/FrameBuilder.h"
#include "jaguar/Hardware.h"
#include "jaguar/Profiler.h"
#include "jaguar/RiscProgram.h"
#include "jaguar/Runtime.h"
#include "jaguar/Video.h"
#include "jaguar/VirtualKeyboard.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace openfranko::src::systems::graphics {
namespace {

namespace jaguar = systems::jaguar;

constexpr std::size_t LIVE_PHRASES = 96;
constexpr int FRAMES = 8;
constexpr int NO_FRAME = -1;
constexpr int COPPER_ENTRY = 1;
constexpr int PANEL_MARGIN = 16;
constexpr unsigned DEBUG_PANEL = 1;
constexpr unsigned KEYBOARD_PANEL = 2;
constexpr unsigned COMPACT_KEYBOARD = 4;
constexpr int COMPACT_PANEL_LEFT = 26;
constexpr int COMPACT_PANEL_BOTTOM = 12;

alignas(jaguar::SCALED_ALIGNMENT) uint64_t liveList[LIVE_PHRASES];
alignas(jaguar::PHRASE_BYTES) uint64_t solidPixels = 0;

struct VblankTarget {
  virtual ~VblankTarget() = default;
  virtual void vblank() = 0;
};

VblankTarget *activeTarget = nullptr;

void onVblank() {
  if (activeTarget) {
    activeTarget->vblank();
  }
}

} // namespace

struct VideoSystem::Window : VblankTarget {
  jaguar::Geometry geometry;
  std::array<jaguar::BuiltFrame, FRAMES> frames;
  std::array<Display, FRAMES> sources;
  std::array<unsigned, FRAMES> sourceOverlays{};
  std::array<bool, FRAMES> built{};
  std::array<uint32_t, FRAMES> used{};
  uint32_t uses = 0;
  volatile int current = NO_FRAME;
  volatile int pending = NO_FRAME;
  volatile uint32_t vbls = 0;
  int appliedClut = NO_FRAME;
  uint32_t copperNext = 0;
  unsigned overlayShown = 0;
  uint32_t statsStart = 0;
  int updates = 0;
  uint32_t slowest = 0;
  uint32_t lastSync = 0;
  uint16_t updateStart = 0;
  uint32_t longestUpdate = 0;

  void vblank() override;
  int backFrame() const;
  void choose(int slot);
  void measure();
};

void VideoSystem::Window::vblank() {
  const int next = pending;
  if (next != NO_FRAME) {
    current = next;
    pending = NO_FRAME;
  }
  const int shown = current;
  if (shown != NO_FRAME) {
    const jaguar::BuiltFrame &frame = frames[static_cast<std::size_t>(shown)];
    const std::size_t phrases = std::min(frame.phrases.size(), LIVE_PHRASES);
    std::memcpy(liveList, frame.phrases.data(), phrases * sizeof(uint64_t));
    const bool copper = frame.copper.size() > COPPER_ENTRY;
    if (copper || appliedClut != shown) {
      volatile uint16_t *clut = &jaguar::word(jaguar::CLUT);
      for (const uint16_t color : frame.clut) {
        *clut++ = color;
      }
      appliedClut = shown;
    }
    jaguar::word(jaguar::BG) = frame.background;
    jaguar::longWord(jaguar::BORD1) =
        (frame.border & 0xFFFF) << 16 | (frame.border >> 16);
    jaguar::longWord(copperNext) =
        reinterpret_cast<uint32_t>(frame.copper.data());
  }
  vbls = vbls + 1;
}

void VideoSystem::Window::measure() {
  const uint32_t now = vbls;
  slowest = std::max(slowest, now - lastSync);
  lastSync = now;
  ++updates;
  const uint32_t elapsed = now - statsStart;
  if (elapsed < static_cast<uint32_t>(geometry.hertz)) {
    return;
  }
  if (jaguar::overlay::isEnabled()) {
    char line[40];
    std::snprintf(line, sizeof(line), "upd %d/%lu slow %lu", updates,
                  static_cast<unsigned long>(elapsed),
                  static_cast<unsigned long>(slowest));
    jaguar::overlay::setLine(0, line);
    std::snprintf(
        line, sizeof(line), "max %lums heap %luK",
        static_cast<unsigned long>(longestUpdate *
                                   jaguar::profiler::TICK_MICROSECONDS / 1000),
        static_cast<unsigned long>(jaguar::runtime::heapPeak() >> 10));
    jaguar::overlay::setLine(1, line);
  }
  statsStart = now;
  updates = 0;
  slowest = 0;
  longestUpdate = 0;
}

int VideoSystem::Window::backFrame() const {
  int oldest = NO_FRAME;
  for (int index = 0; index < FRAMES; ++index) {
    if (index == current || index == pending) {
      continue;
    }
    const std::size_t slot = static_cast<std::size_t>(index);
    if (!built[slot]) {
      return index;
    }
    if (oldest == NO_FRAME ||
        used[slot] < used[static_cast<std::size_t>(oldest)]) {
      oldest = index;
    }
  }
  return oldest == NO_FRAME ? 0 : oldest;
}

void VideoSystem::Window::choose(int slot) {
  used[static_cast<std::size_t>(slot)] = ++uses;
  pending = slot == current ? NO_FRAME : slot;
}

VideoSystem::VideoSystem() : m_window(std::make_unique<Window>()) {
  Window &window = *m_window;
  window.geometry = jaguar::detectGeometry();
  m_ntsc = window.geometry.ntsc;
  const jaguar::RiscProgram gpu = jaguar::gpuProgram();
  jaguar::longWord(jaguar::GPU_CTRL) = 0;
  jaguar::loadProgram(gpu);
  jaguar::blitter::useQueue(gpu.entries[2]);
  window.copperNext = gpu.entries[1];
  jaguar::longWord(jaguar::GPU_PC) = gpu.entries[0];
  jaguar::longWord(jaguar::GPU_CTRL) = jaguar::RISC_GO;

  jaguar::BuiltFrame &blank = window.frames[0];
  jaguar::buildFrame(
      Display{}, window.geometry, reinterpret_cast<uint32_t>(liveList),
      reinterpret_cast<uint32_t>(&solidPixels), nullptr, 0, blank);
  std::memcpy(liveList, blank.phrases.data(),
              blank.phrases.size() * sizeof(uint64_t));
  jaguar::setupVideo(window.geometry);
  jaguar::waitBlanking(window.geometry);
  jaguar::setOlp(reinterpret_cast<uint32_t>(liveList));
  window.current = 0;
  activeTarget = &window;
  jaguar::profiler::start();
  jaguar::runtime::setVideoHandler(onVblank);
  jaguar::runtime::enableVideoInterrupt(window.geometry.lastHalfLine);
}

VideoSystem::~VideoSystem() {
  jaguar::blitter::stopQueue();
  jaguar::runtime::setVideoHandler(nullptr);
  activeTarget = nullptr;
}

void VideoSystem::show(const Display &display) {
  Window &window = *m_window;
  if (display.revision != 0) {
    if (display.revision == m_shownRevision) {
      return;
    }
    for (int slot = 0; slot < FRAMES; ++slot) {
      const std::size_t index = static_cast<std::size_t>(slot);
      if (window.built[index] &&
          window.sources[index].revision == display.revision) {
        m_shownRevision = display.revision;
        m_shownSlot = slot;
        m_frameChanged = true;
        return;
      }
    }
  }
  if (!m_frameChanged && m_shownSlot == NO_FRAME &&
      jaguar::sameLayout(display, m_shown)) {
    return;
  }
  m_shown = display;
  m_shownRevision = display.revision;
  m_shownSlot = NO_FRAME;
  m_frameChanged = true;
}

void VideoSystem::clear() {
  m_shown = Display{};
  m_shownRevision = 0;
  m_shownSlot = NO_FRAME;
  m_frameChanged = true;
  present();
  waitVbl();
}

void VideoSystem::sync() {
  Window &window = *m_window;
  present();
  const uint16_t busy = jaguar::profiler::since(window.updateStart);
  jaguar::profiler::addBusy(busy);
  window.longestUpdate = std::max<uint32_t>(window.longestUpdate, busy);
  waitVbl();
  window.updateStart = jaguar::profiler::now();
  window.measure();
}

void VideoSystem::setNtsc(bool enabled) { m_ntsc = enabled; }

bool VideoSystem::isNtsc() const { return m_ntsc; }

bool VideoSystem::readsBuffersLive() const { return true; }

int VideoSystem::refreshRate() const { return m_window->geometry.hertz; }

void VideoSystem::present() {
  jaguar::blitter::wait();
  Window &window = *m_window;
  const unsigned keyboardPanel = jaguar::keyboard::isCompact()
                                     ? KEYBOARD_PANEL | COMPACT_KEYBOARD
                                     : KEYBOARD_PANEL;
  const unsigned overlay = (jaguar::overlay::isEnabled() ? DEBUG_PANEL : 0) |
                           (jaguar::keyboard::isOpen() ? keyboardPanel : 0);
  if (!m_frameChanged && overlay == window.overlayShown) {
    return;
  }
  m_frameChanged = false;
  window.overlayShown = overlay;
  if (m_shownSlot != NO_FRAME) {
    const std::size_t index = static_cast<std::size_t>(m_shownSlot);
    if (window.built[index] && window.sourceOverlays[index] == overlay &&
        window.sources[index].revision == m_shownRevision) {
      window.choose(m_shownSlot);
      return;
    }
    m_shown = window.sources[index];
    m_shownSlot = NO_FRAME;
  }
  for (int slot = 0; slot < FRAMES; ++slot) {
    const std::size_t index = static_cast<std::size_t>(slot);
    if (window.built[index] && window.sourceOverlays[index] == overlay &&
        jaguar::sameLayout(window.sources[index], m_shown)) {
      window.sources[index].revision = m_shown.revision;
      window.choose(slot);
      return;
    }
  }
  const int back = window.backFrame();
  const std::size_t index = static_cast<std::size_t>(back);
  std::array<jaguar::Overlay, 2> panels;
  std::size_t count = 0;
  if (overlay & DEBUG_PANEL) {
    panels[count++] = {
        reinterpret_cast<uint32_t>(jaguar::overlay::pixels()),
        jaguar::overlay::WIDTH, jaguar::overlay::HEIGHT, PANEL_MARGIN,
        window.geometry.rows - jaguar::overlay::HEIGHT - PANEL_MARGIN};
  }
  if (overlay & KEYBOARD_PANEL) {
    const int width = jaguar::keyboard::width();
    int column = (window.geometry.columns - width) / 2;
    int row = PANEL_MARGIN;
    if (overlay & COMPACT_KEYBOARD) {
      const jaguar::Placement placement =
          jaguar::placeDisplay(m_shown, window.geometry);
      column = placement.left + COMPACT_PANEL_LEFT / placement.halfWidth;
      row = placement.top + m_shown.height / placement.rowsPerLine -
            jaguar::keyboard::HEIGHT - COMPACT_PANEL_BOTTOM;
    }
    panels[count++] = {reinterpret_cast<uint32_t>(jaguar::keyboard::pixels()),
                       width, jaguar::keyboard::HEIGHT, column, row};
  }
  jaguar::buildFrame(m_shown, window.geometry,
                     reinterpret_cast<uint32_t>(liveList),
                     reinterpret_cast<uint32_t>(&solidPixels), panels.data(),
                     count, window.frames[index]);
  if (window.frames[index].phrases.size() > LIVE_PHRASES) {
    throw std::runtime_error("Video system error: too many display layers");
  }
  window.sources[index] = m_shown;
  window.sourceOverlays[index] = overlay;
  window.built[index] = true;
  window.choose(back);
}

void VideoSystem::waitVbl() {
  Window &window = *m_window;
  const uint32_t start = window.vbls;
  while (window.vbls == start) {
  }
}

} // namespace openfranko::src::systems::graphics
