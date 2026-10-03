#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#define private public
#define protected public
#include "../../src/engine/Engine.h"
#undef private
#undef protected
#include "../../src/systems/jaguar/Console.h"
#include "../../src/systems/jaguar/FrameBuilder.h"
#include "../../src/systems/jaguar/Hardware.h"
#include "../../src/systems/jaguar/Profiler.h"
#include "../../src/systems/jaguar/Runtime.h"
#include "memoryCalls.h"
#include "sampler.h"

extern "C" char __text_start[];
extern "C" char __text_end[];

extern "C" int __wrap_getentropy(void *buffer, std::size_t size) {
  static uint32_t state = 0x6A09E667u;
  uint8_t *out = static_cast<uint8_t *>(buffer);
  for (std::size_t at = 0; at < size; ++at) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    out[at] = static_cast<uint8_t>(state >> 24);
  }
  return 0;
}

using namespace openfranko::src;
using namespace openfranko::src::engine;
using namespace openfranko::src::systems;
namespace jag = openfranko::src::systems::jaguar;

namespace {

constexpr uint32_t RUN_VBLS = 600;
constexpr uint32_t WARMUP_VBLS = 1320;
constexpr uint32_t REPORT_VBLS = 300;
constexpr std::size_t HOTSPOTS = 96;
constexpr int PER_ROW = 4;
constexpr uint32_t WATCHDOG_VBLS = 900;
constexpr std::size_t MEMORY_SITES = 48;
constexpr std::size_t MAX_PAGES = 12;
constexpr uint32_t PAGE_VBLS = 150;
constexpr uint32_t WAVE_WARMUP = 900;
constexpr uint32_t WAVE_UPDATES = 1500;
constexpr int BRAWL_CYCLE = 180;
constexpr int BRAWL_WALK = 72;
constexpr int BRAWL_PUNCH = 27;
constexpr int BRAWL_PRESS = 6;
constexpr uint32_t SPAN_BUCKETS = 5;

struct Scenario {
  const char *name;
  states::EngineStateId state;
  int stage;
  int firePeriod;
  bool wiggle;
  int kills = 0;
  bool brawl = false;
  uint32_t runVbls = RUN_VBLS;
  uint32_t warmupUpdates = 0;
  uint32_t runUpdates = 0;
};

void key(Engine &engine, input::Key k, bool pressed) {
  input::KeyEvent event;
  event.key = k;
  event.code = static_cast<int>(k) + 1000;
  event.pressed = pressed;
  engine.m_controllerSystem.receiveKey(event);
}

void waitVbls(uint32_t count) {
  const uint32_t start = jag::runtime::vblCount();
  while (jag::runtime::vblCount() - start < count) {
  }
}

void profile(const Scenario &scenario) {
  uint32_t updates = 0;
  uint32_t vbls = 0;
  uint32_t busyTicks = 0;
  std::array<uint32_t, SPAN_BUCKETS> spans{};
  {
    street::session::GameSession session;
    if (scenario.stage >= 0) {
      session.registers[amal::RO] = static_cast<int16_t>(scenario.stage);
    }
    if (scenario.kills > 0) {
      session.registers[amal::RN] = static_cast<int16_t>(scenario.kills);
    }
    jag::runtime::armWatchdog(WATCHDOG_VBLS);
    Engine engine(scenario.state, std::move(session));
    int frame = 0;
    auto step = [&] {
      if (scenario.firePeriod > 0) {
        const int phase = frame % scenario.firePeriod;
        if (phase == 0) {
          key(engine, input::Key::Space, true);
        } else if (phase == 3) {
          key(engine, input::Key::Space, false);
        }
      }
      if (scenario.wiggle) {
        const int phase = frame % 120;
        key(engine, input::Key::Right, phase < 50);
        key(engine, input::Key::Left, phase >= 70 && phase < 90);
      }
      if (scenario.brawl) {
        const int phase = frame % BRAWL_CYCLE;
        key(engine, input::Key::Right, phase < BRAWL_WALK);
        const int punch = (phase - BRAWL_WALK) % BRAWL_PUNCH;
        key(engine, input::Key::Space,
            phase >= BRAWL_WALK && punch < BRAWL_PRESS);
      }
      engine.isRunning();
      engine.update();
      jag::runtime::feedWatchdog();
      ++frame;
    };
    const uint32_t warm = jag::runtime::vblCount();
    if (scenario.warmupUpdates > 0) {
      for (uint32_t update = 0; update < scenario.warmupUpdates; ++update) {
        step();
      }
    } else {
      while (jag::runtime::vblCount() - warm < WARMUP_VBLS) {
        step();
      }
    }
    const uint32_t busyBefore = jag::profiler::busy();
    const uint32_t start = jag::runtime::vblCount();
#ifndef PROFILE_NO_SAMPLER
    sampler::start(reinterpret_cast<uint32_t>(__text_start),
                   reinterpret_cast<uint32_t>(__text_end));
#endif
    memory_calls::start();
    while (scenario.runUpdates > 0
               ? updates < scenario.runUpdates
               : jag::runtime::vblCount() - start < scenario.runVbls) {
#ifdef PROFILE_SLOW_FRAMES
      sampler::beginFrame();
      const uint32_t before = jag::runtime::vblCount();
      step();
      sampler::endFrame(jag::runtime::vblCount() - before > 1);
#else
      const uint32_t before = jag::runtime::vblCount();
      step();
      const uint32_t taken = jag::runtime::vblCount() - before;
      ++spans[std::min<uint32_t>(taken, SPAN_BUCKETS - 1)];
#endif
      ++updates;
    }
    sampler::stop();
    memory_calls::stop();
    jag::runtime::armWatchdog(0);
    vbls = jag::runtime::vblCount() - start;
    busyTicks = jag::profiler::busy() - busyBefore;
  }
  char line[64];
  const std::vector<sampler::Hotspot> spots =
      sampler::hottest(HOTSPOTS * MAX_PAGES);
  const std::size_t pages =
      std::max<std::size_t>(1, (spots.size() + HOTSPOTS - 1) / HOTSPOTS);
  for (std::size_t page = 0; page < pages; ++page) {
    jag::console::clear();
    std::snprintf(line, sizeof(line), "%s p%u/%u u%lu v%lu b%lu s%lu o%lu\n",
                  scenario.name, static_cast<unsigned>(page + 1),
                  static_cast<unsigned>(pages),
                  static_cast<unsigned long>(updates),
                  static_cast<unsigned long>(vbls),
                  static_cast<unsigned long>(busyTicks *
                                             jag::profiler::TICK_MICROSECONDS /
                                             std::max<uint32_t>(updates, 1)),
                  static_cast<unsigned long>(sampler::total()),
                  static_cast<unsigned long>(sampler::outside()));
    std::string text = line;
    std::snprintf(line, sizeof(line), "vbls 1:%lu 2:%lu 3:%lu 4+:%lu\n",
                  static_cast<unsigned long>(spans[1]),
                  static_cast<unsigned long>(spans[2]),
                  static_cast<unsigned long>(spans[3]),
                  static_cast<unsigned long>(spans[4]));
    text += line;
    const std::size_t first = page * HOTSPOTS;
    const std::size_t last = std::min(spots.size(), first + HOTSPOTS);
    for (std::size_t index = first; index < last; ++index) {
      std::snprintf(line, sizeof(line), "%06lX %5lu",
                    static_cast<unsigned long>(spots[index].address),
                    static_cast<unsigned long>(spots[index].samples));
      text += line;
      text += (index - first + 1) % PER_ROW == 0 ? "\n" : " ";
    }
    jag::console::write(text.data(), text.size());
    jag::console::attach("PROFILE");
    waitVbls(PAGE_VBLS);
  }
  std::string text;
  jag::console::clear();
  std::snprintf(line, sizeof(line), "%s memory u%lu\n", scenario.name,
                static_cast<unsigned long>(updates));
  text = line;
  const std::vector<memory_calls::CallSite> sites =
      memory_calls::heaviest(MEMORY_SITES);
  for (std::size_t index = 0; index < sites.size(); ++index) {
    std::snprintf(line, sizeof(line), "%06lX %c %8lu %6lu",
                  static_cast<unsigned long>(sites[index].caller),
                  static_cast<char>(sites[index].kind),
                  static_cast<unsigned long>(sites[index].bytes),
                  static_cast<unsigned long>(sites[index].calls));
    text += line;
    text += index % 2 == 1 ? "\n" : "  ";
  }
  jag::console::write(text.data(), text.size());
  jag::console::attach("MEMORY");
  waitVbls(REPORT_VBLS);
  jag::console::clear();
  std::snprintf(line, sizeof(line), "%s arithmetic\n", scenario.name);
  text = line;
  const std::vector<memory_calls::CallSite> helpers =
      memory_calls::busiestArithmetic(MEMORY_SITES);
  for (std::size_t index = 0; index < helpers.size(); ++index) {
    std::snprintf(line, sizeof(line), "%06lX %c %8lu",
                  static_cast<unsigned long>(helpers[index].caller),
                  static_cast<char>(helpers[index].kind),
                  static_cast<unsigned long>(helpers[index].calls));
    text += line;
    text += index % 2 == 1 ? "\n" : "  ";
  }
  jag::console::write(text.data(), text.size());
  jag::console::attach("ARITH");
  waitVbls(REPORT_VBLS);
}

} // namespace

int main() {
  using states::EngineStateId;
#ifdef PROFILE_FAST_ROM
  jag::word(jag::MEMCON1) =
      static_cast<uint16_t>((jag::word(jag::MEMCON1) & ~0x18) | 0x18);
#endif
#ifndef PROFILE_COPPER
  jag::allowCopper(false);
#endif
  const std::vector<Scenario> scenarios = {
      {"mirage", EngineStateId::Mirage, -1, 0, false},
      {"title", EngineStateId::TitleAndStory, -1, 150, false},
      {"menu", EngineStateId::Menu, -1, 0, false},
      {"level1", EngineStateId::Level1, 0, 9, true},
      {"wave", EngineStateId::Level1, 0, 0, false, 0, true, 0, WAVE_WARMUP,
       WAVE_UPDATES},
      {"car1", EngineStateId::Level1Car, 1, 0, true},
      {"level2", EngineStateId::Level2, 1, 9, true},
      {"level3", EngineStateId::Level3, 2, 9, true},
      {"highscore", EngineStateId::HighScore, -1, 0, false, 50},
  };
  for (const Scenario &scenario : scenarios) {
#ifdef PROFILE_ONLY
    if (std::strcmp(scenario.name, PROFILE_ONLY) != 0) {
      continue;
    }
#endif
    profile(scenario);
  }
  jag::console::show("PROFILE DONE", false);
}
