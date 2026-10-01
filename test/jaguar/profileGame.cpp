#include <algorithm>
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
#include "../../src/systems/jaguar/Runtime.h"
#include "memoryCalls.h"
#include "sampler.h"

using namespace openfranko::src;
using namespace openfranko::src::engine;
using namespace openfranko::src::systems;
namespace jag = openfranko::src::systems::jaguar;

namespace {

constexpr uint32_t CODE_START = 0x802000;
constexpr uint32_t RUN_VBLS = 600;
constexpr uint32_t WARMUP_VBLS = 1320;
constexpr uint32_t REPORT_VBLS = 300;
constexpr std::size_t HOTSPOTS = 96;
constexpr int PER_ROW = 4;
constexpr uint32_t WATCHDOG_VBLS = 900;
constexpr std::size_t MEMORY_SITES = 48;
constexpr std::size_t MAX_PAGES = 12;
constexpr uint32_t PAGE_VBLS = 150;

struct Scenario {
  const char *name;
  states::EngineStateId state;
  int stage;
  int firePeriod;
  bool wiggle;
  int kills = 0;
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
      engine.isRunning();
      engine.update();
      jag::runtime::feedWatchdog();
      ++frame;
    };
    const uint32_t warm = jag::runtime::vblCount();
    while (jag::runtime::vblCount() - warm < WARMUP_VBLS) {
      step();
    }
    const uint32_t start = jag::runtime::vblCount();
#ifndef PROFILE_NO_SAMPLER
    sampler::start(CODE_START, jag::runtime::romEnd());
#endif
    memory_calls::start();
    while (jag::runtime::vblCount() - start < RUN_VBLS) {
#ifdef PROFILE_SLOW_FRAMES
      sampler::beginFrame();
      const uint32_t before = jag::runtime::vblCount();
      step();
      sampler::endFrame(jag::runtime::vblCount() - before > 1);
#else
      step();
#endif
      ++updates;
    }
    sampler::stop();
    memory_calls::stop();
    jag::runtime::armWatchdog(0);
    vbls = jag::runtime::vblCount() - start;
  }
  char line[64];
  const std::vector<sampler::Hotspot> spots =
      sampler::hottest(HOTSPOTS * MAX_PAGES);
  const std::size_t pages =
      std::max<std::size_t>(1, (spots.size() + HOTSPOTS - 1) / HOTSPOTS);
  for (std::size_t page = 0; page < pages; ++page) {
    jag::console::clear();
    std::snprintf(
        line, sizeof(line), "%s p%u/%u u%lu v%lu s%lu o%lu\n", scenario.name,
        static_cast<unsigned>(page + 1), static_cast<unsigned>(pages),
        static_cast<unsigned long>(updates), static_cast<unsigned long>(vbls),
        static_cast<unsigned long>(sampler::total()),
        static_cast<unsigned long>(sampler::outside()));
    std::string text = line;
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
}

} // namespace

int main() {
  using states::EngineStateId;
#ifndef PROFILE_COPPER
  jag::allowCopper(false);
#endif
  const std::vector<Scenario> scenarios = {
      {"mirage", EngineStateId::Mirage, -1, 0, false},
      {"title", EngineStateId::TitleAndStory, -1, 150, false},
      {"menu", EngineStateId::Menu, -1, 0, false},
      {"level1", EngineStateId::Level1, 0, 9, true},
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
