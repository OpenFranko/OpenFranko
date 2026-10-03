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
#include <memory>
#include <new>
#include <stdexcept>
#include <vector>

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
constexpr uint64_t BYTE_COPIES = 0x0101010101010101ull;

alignas(jaguar::PHRASE_BYTES) uint64_t solidPhrases[jaguar::SOLID_PHRASES];
alignas(jaguar::PHRASE_BYTES) uint64_t maskPhrases[FRAMES];

struct VblankTarget {
  virtual ~VblankTarget() = default;
  virtual void vblank() = 0;
};

VblankTarget *activeTarget = nullptr;

void takeSprites(Display &target, const Display &source) {
  target.revision = source.revision;
  const Layer *from = source.layers.data();
  for (Layer &layer : target.layers) {
    if (layer.carriesSprites) {
      layer.sprites = from->sprites;
    }
    ++from;
  }
}

void takeSprites(Display &target, const jaguar::SpriteLists &sprites) {
  const std::vector<Sprite> *from = sprites.data();
  for (Layer &layer : target.layers) {
    if (layer.carriesSprites) {
      layer.sprites = *from;
    }
    ++from;
  }
}
bool gpuRunning = false;

void onVblank() {
  if (activeTarget) {
    activeTarget->vblank();
  }
}

} // namespace

struct TranslationBuffer {
  const uint8_t *source = nullptr;
  std::size_t bytes = 0;
  std::unique_ptr<uint64_t[]> storage;
  uint32_t revision = 0;
};

struct SourceRevision {
  const uint8_t *source = nullptr;
  uint32_t revision = 0;
};

struct VideoSystem::Window : VblankTarget, jaguar::TranslationBuffers {
  jaguar::Geometry geometry;
  std::array<jaguar::BuiltFrame, FRAMES> frames;
  std::array<Display, FRAMES> sources;
  std::array<unsigned, FRAMES> sourceOverlays{};
  std::array<bool, FRAMES> built{};
  std::array<uint32_t, FRAMES> used{};
  uint32_t uses = 0;
  jaguar::SpriteLists wanted;
  volatile int current = NO_FRAME;
  volatile int pending = NO_FRAME;
  volatile bool marked = true;
  volatile uint32_t frameMark = 0;
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
  std::vector<TranslationBuffer> buffers;
  std::array<std::unique_ptr<uint64_t[]>, FRAMES> lines;
  std::array<SourceRevision, FRAMES> revisions{};

  void vblank() override;
  uint8_t *buffer(const uint8_t *source, std::size_t bytes) override;
  bool isReferenced(const uint64_t *storage) const;
  void refreshTranslations();
  void noteRevisions(const Display &display);
  uint32_t revisionOf(const uint8_t *source) const;
  void prepareLines(std::size_t slot, const Display &display);
  int lineCapacity() const { return geometry.rows + 2; }
  jaguar::FrameMemory memory(std::size_t slot) {
    return {reinterpret_cast<uint32_t>(liveList),
            reinterpret_cast<uint32_t>(solidPhrases),
            this,
            reinterpret_cast<uint8_t *>(lines[slot].get()),
            lines[slot] ? lineCapacity() : 0,
            reinterpret_cast<uint8_t *>(&maskPhrases[slot])};
  }
  int backFrame() const;
  void want(const Display &display);
  int recentFrame(unsigned overlay) const;
  int twinFrame(int slot, uint32_t revision, unsigned overlay) const;
  void build(int slot, const Display &shown, unsigned overlay);
  void choose(int slot);
  void measure();
};

void VideoSystem::Window::vblank() {
  const int next = pending;
  if (next != NO_FRAME && marked && jaguar::blitter::reached(frameMark)) {
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

uint8_t *VideoSystem::Window::buffer(const uint8_t *source, std::size_t bytes) {
  for (TranslationBuffer &entry : buffers) {
    if (entry.source == source && entry.bytes == bytes) {
      return reinterpret_cast<uint8_t *>(entry.storage.get());
    }
  }
  buffers.erase(std::remove_if(buffers.begin(), buffers.end(),
                               [this](const TranslationBuffer &entry) {
                                 return !isReferenced(entry.storage.get());
                               }),
                buffers.end());
  TranslationBuffer entry;
  entry.source = source;
  entry.bytes = bytes;
  entry.storage.reset(
      new (std::nothrow)
          uint64_t[(bytes + sizeof(uint64_t) - 1) / sizeof(uint64_t)]);
  if (!entry.storage) {
    return nullptr;
  }
  uint8_t *storage = reinterpret_cast<uint8_t *>(entry.storage.get());
  buffers.push_back(std::move(entry));
  return storage;
}

bool VideoSystem::Window::isReferenced(const uint64_t *storage) const {
  const uint8_t *target = reinterpret_cast<const uint8_t *>(storage);
  for (std::size_t slot = 0; slot < frames.size(); ++slot) {
    if (!built[slot]) {
      continue;
    }
    for (const jaguar::Translation &translation : frames[slot].translations) {
      if (translation.target == target) {
        return true;
      }
    }
  }
  return false;
}

void VideoSystem::Window::refreshTranslations() {
  const int slot = pending != NO_FRAME ? pending : current;
  if (slot == NO_FRAME) {
    return;
  }
  for (const jaguar::Translation &translation :
       frames[static_cast<std::size_t>(slot)].translations) {
    const uint32_t revision = revisionOf(translation.source);
    TranslationBuffer *target = nullptr;
    for (TranslationBuffer &entry : buffers) {
      if (reinterpret_cast<uint8_t *>(entry.storage.get()) ==
          translation.target) {
        target = &entry;
      }
    }
    if (revision != 0 && target && target->revision == revision) {
      continue;
    }
    if (!jaguar::blitter::translate(translation.source, translation.target,
                                    translation.bytes, translation.keep,
                                    translation.flip)) {
      jaguar::translateOnCpu(translation);
    }
    if (target) {
      target->revision = revision;
    }
  }
}

void VideoSystem::Window::noteRevisions(const Display &display) {
  for (const Layer &layer : display.layers) {
    if (layer.revision == 0 || !layer.pixels) {
      continue;
    }
    SourceRevision *slot = &revisions[0];
    for (SourceRevision &entry : revisions) {
      if (entry.source == layer.pixels) {
        slot = &entry;
        break;
      }
      if (entry.revision < slot->revision) {
        slot = &entry;
      }
    }
    slot->source = layer.pixels;
    slot->revision = layer.revision;
  }
}

uint32_t VideoSystem::Window::revisionOf(const uint8_t *source) const {
  for (const SourceRevision &entry : revisions) {
    if (entry.source == source) {
      return entry.revision;
    }
  }
  return 0;
}

void VideoSystem::Window::prepareLines(std::size_t slot,
                                       const Display &display) {
  const bool needed =
      std::any_of(display.layers.begin(), display.layers.end(),
                  [](const Layer &layer) { return !layer.rowColors.empty(); });
  if (needed == static_cast<bool>(lines[slot])) {
    return;
  }
  frames[slot].lineTarget = nullptr;
  if (needed) {
    lines[slot].reset(new (std::nothrow)
                          uint64_t[static_cast<std::size_t>(lineCapacity())]);
  } else {
    lines[slot].reset();
  }
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

void VideoSystem::Window::build(int slot, const Display &shown,
                                unsigned overlay) {
  const std::size_t index = static_cast<std::size_t>(slot);
  std::array<jaguar::Overlay, 2> panels;
  std::size_t count = 0;
  if (overlay & DEBUG_PANEL) {
    panels[count++] = {reinterpret_cast<uint32_t>(jaguar::overlay::pixels()),
                       jaguar::overlay::WIDTH, jaguar::overlay::HEIGHT,
                       PANEL_MARGIN,
                       geometry.rows - jaguar::overlay::HEIGHT - PANEL_MARGIN};
  }
  if (overlay & KEYBOARD_PANEL) {
    const int width = jaguar::keyboard::width();
    int column = (geometry.columns - width) / 2;
    int row = PANEL_MARGIN;
    if (overlay & COMPACT_KEYBOARD) {
      const jaguar::Placement placement = jaguar::placeDisplay(shown, geometry);
      column = placement.left + COMPACT_PANEL_LEFT / placement.halfWidth;
      row = placement.top + shown.height / placement.rowsPerLine -
            jaguar::keyboard::HEIGHT - COMPACT_PANEL_BOTTOM;
    }
    panels[count++] = {reinterpret_cast<uint32_t>(jaguar::keyboard::pixels()),
                       width, jaguar::keyboard::HEIGHT, column, row};
  }
  prepareLines(index, shown);
  jaguar::buildFrame(shown, geometry, memory(index), panels.data(), count,
                     frames[index]);
}

void VideoSystem::Window::want(const Display &display) {
  if (wanted.size() != display.layers.size()) {
    wanted.resize(display.layers.size());
  }
  std::vector<Sprite> *to = wanted.data();
  for (const Layer &layer : display.layers) {
    if (layer.carriesSprites) {
      *to = layer.sprites;
    } else {
      to->clear();
    }
    ++to;
  }
}

int VideoSystem::Window::recentFrame(unsigned overlay) const {
  int recent = NO_FRAME;
  for (int index = 0; index < FRAMES; ++index) {
    const std::size_t slot = static_cast<std::size_t>(index);
    if (index == current || index == pending || !built[slot] ||
        sourceOverlays[slot] != overlay) {
      continue;
    }
    if (recent == NO_FRAME ||
        used[slot] > used[static_cast<std::size_t>(recent)]) {
      recent = index;
    }
  }
  return recent;
}

int VideoSystem::Window::twinFrame(int slot, uint32_t revision,
                                   unsigned overlay) const {
  for (int index = 0; index < FRAMES; ++index) {
    const std::size_t at = static_cast<std::size_t>(index);
    if (index != slot && index != current && index != pending && built[at] &&
        sourceOverlays[at] == overlay && sources[at].revision == revision) {
      return index;
    }
  }
  return NO_FRAME;
}

void VideoSystem::Window::choose(int slot) {
  marked = false;
  used[static_cast<std::size_t>(slot)] = ++uses;
  pending = slot == current ? NO_FRAME : slot;
}

VideoSystem::VideoSystem() : m_window(std::make_unique<Window>()) {
  Window &window = *m_window;
  window.geometry = jaguar::detectGeometry();
  m_ntsc = window.geometry.ntsc;
  const jaguar::RiscProgram gpu = jaguar::gpuProgram();
  if (gpuRunning) {
    jaguar::blitter::resumeQueue(gpu.entries[2]);
  } else {
    jaguar::longWord(jaguar::GPU_CTRL) = 0;
    jaguar::loadProgram(gpu);
    jaguar::blitter::useQueue(gpu.entries[2]);
    jaguar::longWord(jaguar::GPU_PC) = gpu.entries[0];
    jaguar::longWord(jaguar::GPU_CTRL) = jaguar::RISC_GO;
    gpuRunning = true;
  }
  window.copperNext = gpu.entries[1];

  for (int value = 0; value < jaguar::SOLID_PHRASES; ++value) {
    solidPhrases[value] = static_cast<uint64_t>(value) * BYTE_COPIES;
  }
  jaguar::allowCopper(false);
  jaguar::BuiltFrame &blank = window.frames[0];
  jaguar::buildFrame(Display{}, window.geometry, window.memory(0), nullptr, 0,
                     blank);
  std::memcpy(liveList, blank.phrases.data(),
              blank.phrases.size() * sizeof(uint64_t));
  jaguar::setupVideo(window.geometry);
  jaguar::waitTopBlanking(window.geometry);
  jaguar::setOlp(reinterpret_cast<uint32_t>(liveList));
  window.current = 0;
  activeTarget = &window;
  jaguar::profiler::start();
  jaguar::runtime::setVideoHandler(onVblank);
  jaguar::runtime::enableVideoInterrupt(window.geometry.vblankHalfLine);
}

VideoSystem::~VideoSystem() {
  jaguar::blitter::stopQueue();
  jaguar::runtime::setVideoHandler(nullptr);
  activeTarget = nullptr;
}

void VideoSystem::show(const Display &display) {
  Window &window = *m_window;
  window.noteRevisions(display);
  if (display.revision != 0) {
    if (display.revision == m_shownRevision) {
      if (m_shownSlot != NO_FRAME) {
        if (!jaguar::sameSprites(display, window.wanted)) {
          window.want(display);
          m_frameChanged = true;
        }
      } else if (!jaguar::sameSprites(display, m_shown)) {
        takeSprites(m_shown, display);
        m_frameChanged = true;
      }
      return;
    }
    for (int slot = 0; slot < FRAMES; ++slot) {
      const std::size_t index = static_cast<std::size_t>(slot);
      if (window.built[index] &&
          window.sources[index].revision == display.revision) {
        m_shownRevision = display.revision;
        m_shownSlot = slot;
        window.want(display);
        m_frameChanged = true;
        return;
      }
    }
  }
  if (!m_frameChanged && m_shownSlot == NO_FRAME &&
      jaguar::sameLayout(display, m_shown)) {
    return;
  }
  assign(m_shown, display);
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
  jaguar::blitter::wait();
  window.measure();
}

void VideoSystem::setNtsc(bool enabled) { m_ntsc = enabled; }

bool VideoSystem::isNtsc() const { return m_ntsc; }

bool VideoSystem::readsBuffersLive() const { return true; }

bool VideoSystem::showsSprites() const { return true; }

int VideoSystem::refreshRate() const { return m_window->geometry.hertz; }

void VideoSystem::present() {
  Window &window = *m_window;
  struct Refresh {
    Window &window;
    ~Refresh() {
      window.refreshTranslations();
      window.frameMark = jaguar::blitter::mark();
      window.marked = true;
    }
  } refresh{window};
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
  window.pending = NO_FRAME;
  if (m_shownSlot != NO_FRAME) {
    const int slot = m_shownSlot;
    const std::size_t index = static_cast<std::size_t>(slot);
    if (window.built[index] && window.sourceOverlays[index] == overlay &&
        window.sources[index].revision == m_shownRevision) {
      if (jaguar::sameSprites(window.sources[index], window.wanted)) {
        window.choose(slot);
        return;
      }
      const int target = slot != window.current
                             ? slot
                             : window.twinFrame(slot, m_shownRevision, overlay);
      if (target != NO_FRAME) {
        const std::size_t at = static_cast<std::size_t>(target);
        if (jaguar::moveSprites(window.sources[at], window.wanted,
                                window.geometry, window.memory(at),
                                window.frames[at])) {
          takeSprites(window.sources[at], window.wanted);
          m_shownSlot = target;
          window.choose(target);
          return;
        }
        window.built[at] = false;
      }
    }
    assign(m_shown, window.sources[index]);
    takeSprites(m_shown, window.wanted);
    m_shownSlot = NO_FRAME;
  }
  int restage = NO_FRAME;
  for (int slot = 0; slot < FRAMES; ++slot) {
    const std::size_t index = static_cast<std::size_t>(slot);
    if (!window.built[index] || window.sourceOverlays[index] != overlay ||
        !jaguar::sameLayers(window.sources[index], m_shown)) {
      continue;
    }
    if (jaguar::sameSprites(window.sources[index], m_shown)) {
      window.sources[index].revision = m_shown.revision;
      settle(slot);
      return;
    }
    if (restage == NO_FRAME && slot != window.current &&
        slot != window.pending) {
      restage = slot;
    }
  }
  if (restage != NO_FRAME) {
    const std::size_t index = static_cast<std::size_t>(restage);
    if (jaguar::moveSprites(m_shown, window.sources[index], window.geometry,
                            window.memory(index), window.frames[index])) {
      takeSprites(window.sources[index], m_shown);
      window.sources[index].revision = m_shown.revision;
      settle(restage);
      return;
    }
    window.built[index] = false;
  }
  for (int slot = 0; slot < FRAMES; ++slot) {
    const std::size_t index = static_cast<std::size_t>(slot);
    if (slot == window.current || !window.built[index] ||
        window.sourceOverlays[index] != overlay ||
        !jaguar::recolorFrame(m_shown, window.sources[index], window.geometry,
                              window.memory(index), window.frames[index])) {
      continue;
    }
    if (!jaguar::sameSprites(window.sources[index], m_shown) &&
        !jaguar::moveSprites(m_shown, window.sources[index], window.geometry,
                             window.memory(index), window.frames[index])) {
      window.built[index] = false;
      continue;
    }
    assign(window.sources[index], m_shown);
    settle(slot);
    return;
  }
  int target = window.recentFrame(overlay);
  bool scrolled = false;
  if (target != NO_FRAME) {
    const std::size_t recent = static_cast<std::size_t>(target);
    window.prepareLines(recent, m_shown);
    scrolled =
        jaguar::scrollFrame(m_shown, window.sources[recent], window.geometry,
                            window.memory(recent), window.frames[recent]);
    if (!scrolled) {
      window.built[recent] = false;
    }
  }
  if (!scrolled) {
    target = window.backFrame();
    window.build(target, m_shown, overlay);
  }
  const std::size_t index = static_cast<std::size_t>(target);
  if (window.frames[index].phrases.size() > LIVE_PHRASES) {
    throw std::runtime_error("Video system error: too many display layers");
  }
  assign(window.sources[index], m_shown);
  window.sourceOverlays[index] = overlay;
  window.built[index] = true;
  settle(target);
}

void VideoSystem::settle(int slot) {
  Window &window = *m_window;
  window.choose(slot);
  if (m_shownRevision != 0) {
    m_shownSlot = slot;
    window.want(m_shown);
  }
}

void VideoSystem::waitVbl() {
  Window &window = *m_window;
  const uint32_t start = window.vbls;
  while (window.vbls == start) {
  }
}

} // namespace openfranko::src::systems::graphics
