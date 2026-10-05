#include "../../../../src/systems/audio/Tracker.h"
#include "../../../../src/systems/audio/S3m.h"

#include <catch2/catch_all.hpp>

#include <xmp.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace openfranko::src::systems::audio;

namespace {

constexpr int RATE = 22050;
constexpr uint8_t NO_NOTE = 255;
constexpr uint8_t NOTE_OFF = 254;
constexpr uint8_t C4 = 0x40;

struct SampleSpec {
  std::vector<uint8_t> data;
  uint32_t loopStart = 0;
  uint32_t loopEnd = 0;
  bool looped = false;
  int volume = 64;
  int c2spd = 8363;
};

struct EventSpec {
  int row = 0;
  int channel = 0;
  uint8_t note = NO_NOTE;
  uint8_t instrument = 0;
  int volume = -1;
  uint8_t command = 0;
  uint8_t parameter = 0;
};

struct ModuleSpec {
  int speed = 6;
  int tempo = 125;
  std::vector<uint8_t> orders;
  std::vector<std::vector<EventSpec>> patterns;
  std::vector<SampleSpec> samples;
};

void put16(std::vector<uint8_t> &file, std::size_t at, uint32_t value) {
  file[at] = static_cast<uint8_t>(value);
  file[at + 1] = static_cast<uint8_t>(value >> 8);
}

void put32(std::vector<uint8_t> &file, std::size_t at, uint32_t value) {
  put16(file, at, value & 0xFFFF);
  put16(file, at + 2, value >> 16);
}

std::size_t paragraph(std::vector<uint8_t> &file) {
  file.resize((file.size() + 15) / 16 * 16);
  return file.size() / 16;
}

std::vector<uint8_t> buildS3m(const ModuleSpec &spec) {
  std::vector<uint8_t> file(0x60, 0);
  file[0x1C] = 0x1A;
  file[0x1D] = 16;
  put16(file, 0x20, static_cast<uint32_t>(spec.orders.size()));
  put16(file, 0x22, static_cast<uint32_t>(spec.samples.size()));
  put16(file, 0x24, static_cast<uint32_t>(spec.patterns.size()));
  put16(file, 0x28, 0x1320);
  put16(file, 0x2A, 2);
  file[0x2C] = 'S';
  file[0x2D] = 'C';
  file[0x2E] = 'R';
  file[0x2F] = 'M';
  file[0x30] = 64;
  file[0x31] = static_cast<uint8_t>(spec.speed);
  file[0x32] = static_cast<uint8_t>(spec.tempo);
  file[0x33] = 0xB0;
  file[0x34] = 16;
  file[0x35] = 0xFC;
  const std::array<uint8_t, 4> channels = {0, 8, 9, 1};
  for (std::size_t channel = 0; channel < 32; ++channel) {
    file[0x40 + channel] = channel < 4 ? channels[channel] : 255;
  }
  file.insert(file.end(), spec.orders.begin(), spec.orders.end());
  const std::size_t pointers = file.size();
  file.resize(file.size() + 2 * (spec.samples.size() + spec.patterns.size()));
  const std::array<uint8_t, 4> pans = {0x23, 0x2C, 0x2C, 0x23};
  for (std::size_t channel = 0; channel < 32; ++channel) {
    file.push_back(channel < 4 ? pans[channel] : 0);
  }
  for (std::size_t index = 0; index < spec.samples.size(); ++index) {
    const SampleSpec &sample = spec.samples[index];
    const std::size_t header = paragraph(file);
    put16(file, pointers + 2 * index, static_cast<uint32_t>(header));
    file.resize(file.size() + 0x50, 0);
    const std::size_t at = header * 16;
    file[at] = 1;
    put32(file, at + 16, static_cast<uint32_t>(sample.data.size()));
    put32(file, at + 20, sample.loopStart);
    put32(file, at + 24, sample.loopEnd);
    file[at + 28] = static_cast<uint8_t>(sample.volume);
    file[at + 31] = sample.looped ? 1 : 0;
    put32(file, at + 32, static_cast<uint32_t>(sample.c2spd));
    file[at + 76] = 'S';
    file[at + 77] = 'C';
    file[at + 78] = 'R';
    file[at + 79] = 'S';
    const std::size_t data = paragraph(file);
    file[at + 13] = static_cast<uint8_t>(data >> 16);
    put16(file, at + 14, static_cast<uint32_t>(data & 0xFFFF));
    file.insert(file.end(), sample.data.begin(), sample.data.end());
  }
  for (std::size_t index = 0; index < spec.patterns.size(); ++index) {
    const std::size_t start = paragraph(file);
    put16(file, pointers + 2 * (spec.samples.size() + index),
          static_cast<uint32_t>(start));
    const std::size_t at = file.size();
    file.resize(file.size() + 2, 0);
    for (int row = 0; row < S3mModule::ROWS; ++row) {
      for (const EventSpec &event : spec.patterns[index]) {
        if (event.row != row) {
          continue;
        }
        uint8_t what = static_cast<uint8_t>(event.channel);
        if (event.note != NO_NOTE || event.instrument != 0) {
          what |= 0x20;
        }
        if (event.volume >= 0) {
          what |= 0x40;
        }
        if (event.command != 0) {
          what |= 0x80;
        }
        file.push_back(what);
        if (what & 0x20) {
          file.push_back(event.note);
          file.push_back(event.instrument);
        }
        if (what & 0x40) {
          file.push_back(static_cast<uint8_t>(event.volume));
        }
        if (what & 0x80) {
          file.push_back(event.command);
          file.push_back(event.parameter);
        }
      }
      file.push_back(0);
    }
    put16(file, at, static_cast<uint32_t>(file.size() - at));
  }
  return file;
}

std::vector<uint8_t> wave(std::size_t length, int period, int amplitude) {
  std::vector<uint8_t> data(length);
  for (std::size_t at = 0; at < length; ++at) {
    const double phase = 6.283185307179586 * static_cast<double>(at) / period;
    data[at] =
        static_cast<uint8_t>(128 + std::lround(amplitude * std::sin(phase)));
  }
  return data;
}

ModuleSpec song() {
  ModuleSpec spec;
  spec.speed = 3;
  spec.tempo = 125;
  spec.orders = {0, 1, 255};
  SampleSpec tone;
  tone.data = wave(4000, 40, 90);
  tone.volume = 50;
  tone.c2spd = 8287;
  SampleSpec loop;
  loop.data = wave(1200, 25, 70);
  loop.looped = true;
  loop.loopStart = 200;
  loop.loopEnd = 1200;
  loop.volume = 64;
  spec.samples = {tone, loop};
  spec.patterns = {
      {
          {0, 0, C4, 1, -1, 0, 0},
          {0, 1, 0x47, 2, 40, 0, 0},
          {4, 2, 0x34, 1, -1, 6, 0x04},
          {8, 3, 0x40, 2, -1, 10, 0x37},
          {12, 0, NOTE_OFF, 0, -1, 0, 0},
          {16, 1, NO_NOTE, 0, 20, 1, 4},
          {20, 2, 0x49, 2, 63, 20, 0x90},
          {24, 0, 0x45, 1, -1, 3, 0},
      },
      {
          {0, 0, 0x50, 2, -1, 0, 0},
          {2, 1, 0x42, 1, -1, 0, 0},
          {6, 3, 0x37, 1, 30, 0, 0},
          {10, 2, NOTE_OFF, 0, -1, 0, 0},
          {14, 0, NO_NOTE, 0, -1, 3, 0},
      },
  };
  return spec;
}

struct Voice {
  const S3mSample *sample = nullptr;
  uint64_t position = 0;
  uint64_t step = 0;
  int gainLeft = 0;
  int gainRight = 0;
  bool active = false;
};

std::vector<int16_t> renderTracker(const S3mModule &module) {
  Tracker tracker(module);
  std::array<Voice, S3mModule::CHANNELS> voices;
  std::vector<int16_t> out;
  while (tracker.loops() == 0) {
    const TrackerTick &tick = tracker.advance();
    for (std::size_t index = 0; index < voices.size(); ++index) {
      const TrackerVoice &state = tick.voices[index];
      Voice &voice = voices[index];
      if (state.trigger && state.sample >= 0) {
        voice.sample = &module.samples[static_cast<std::size_t>(state.sample)];
        voice.position = 0;
        voice.active = true;
      }
      if (!state.active) {
        voice.active = false;
      }
      voice.step = static_cast<uint64_t>(
          Tracker::C4_PERIOD * static_cast<double>(Tracker::C4_RATE) *
          Tracker::PERIOD_ONE / state.period / RATE * 4294967296.0);
      voice.gainLeft = state.gainLeft;
      voice.gainRight = state.gainRight;
    }
    const int frames = static_cast<int>(RATE * 2.5 / tick.bpm);
    for (int frame = 0; frame < frames; ++frame) {
      int32_t left = 0;
      int32_t right = 0;
      for (Voice &voice : voices) {
        if (!voice.active) {
          continue;
        }
        const S3mSample &sample = *voice.sample;
        const std::size_t index =
            static_cast<std::size_t>(voice.position >> 32);
        if (index >= sample.data.size()) {
          voice.active = false;
          continue;
        }
        std::size_t next = index + 1;
        if (sample.looped && next >= sample.loopEnd) {
          next = sample.loopStart;
        }
        const int first = sample.data[index] * 256;
        const int second =
            next < sample.data.size() ? sample.data[next] * 256 : first;
        const int fraction = static_cast<int>((voice.position >> 16) & 0xFFFF);
        const int value = first + (((fraction >> 1) * (second - first)) >> 15);
        left += value * voice.gainLeft;
        right += value * voice.gainRight;
        voice.position += voice.step;
        while (sample.looped && (voice.position >> 32) >= sample.loopEnd) {
          voice.position -=
              static_cast<uint64_t>(sample.loopEnd - sample.loopStart) << 32;
        }
      }
      out.push_back(
          static_cast<int16_t>(std::clamp(left >> 11, -32768, 32767)));
      out.push_back(
          static_cast<int16_t>(std::clamp(right >> 11, -32768, 32767)));
    }
  }
  return out;
}

std::vector<int16_t> renderXmp(const std::vector<uint8_t> &file) {
  xmp_context context = xmp_create_context();
  REQUIRE(xmp_load_module_from_memory(context, file.data(),
                                      static_cast<long>(file.size())) == 0);
  REQUIRE(xmp_start_player(context, RATE, 0) == 0);
  std::vector<int16_t> out;
  std::vector<int16_t> chunk(1024);
  while (xmp_play_buffer(context, chunk.data(),
                         static_cast<int>(chunk.size() * sizeof(int16_t)),
                         1) == 0) {
    out.insert(out.end(), chunk.begin(), chunk.end());
  }
  xmp_end_player(context);
  xmp_release_module(context);
  xmp_free_context(context);
  return out;
}

double correlation(const std::vector<int16_t> &first,
                   const std::vector<int16_t> &second) {
  const std::size_t size = std::min(first.size(), second.size());
  double cross = 0.0;
  double energyFirst = 0.0;
  double energySecond = 0.0;
  for (std::size_t at = 0; at < size; ++at) {
    cross += static_cast<double>(first[at]) * second[at];
    energyFirst += static_cast<double>(first[at]) * first[at];
    energySecond += static_cast<double>(second[at]) * second[at];
  }
  return cross / std::sqrt(energyFirst * energySecond);
}

constexpr uint8_t SIGN_BIT = 0x80;
constexpr int8_t FLIPPED = 7;

bool flipToConstant(const uint8_t *, int8_t *target, std::size_t size) {
  std::fill(target, target + size, FLIPPED);
  return true;
}

bool refuseFlip(const uint8_t *, int8_t *, std::size_t) { return false; }

std::vector<int8_t> converted(const std::vector<uint8_t> &source,
                              uint8_t flip) {
  std::vector<int8_t> data;
  for (const uint8_t value : source) {
    data.push_back(static_cast<int8_t>(value ^ flip));
  }
  return data;
}

} // namespace

SCENARIO("An S3M module is read into patterns, samples and pans") {
  GIVEN("A two-pattern module with a looped and an unlooped sample") {
    S3mModule module;
    REQUIRE(parseS3m(buildS3m(song()), module));

    THEN("Speed, tempo and the order list come from the header") {
      REQUIRE(module.speed == 3);
      REQUIRE(module.tempo == 125);
      REQUIRE(module.orders == std::vector<uint8_t>{0, 1, 255});
    }

    THEN("Unsigned sample data becomes signed and loops are kept") {
      REQUIRE(module.samples.size() == 2);
      REQUIRE(module.samples[0].data.size() == 4000);
      REQUIRE(module.samples[0].data[0] == 0);
      REQUIRE_FALSE(module.samples[0].looped);
      REQUIRE(module.samples[0].volume == 50);
      REQUIRE(module.samples[1].looped);
      REQUIRE(module.samples[1].loopStart == 200);
      REQUIRE(module.samples[1].loopEnd == 1200);
    }

    THEN("The pan table places channels left, right, right, left") {
      REQUIRE(module.pans == std::array<int, 4>{0x30, 0xC0, 0xC0, 0x30});
    }

    THEN("Rows with speed or tempo commands are remembered") {
      REQUIRE(module.tempoRows ==
              std::set<std::pair<int, int>>{{0, 16}, {0, 20}});
    }

    THEN("A C2Spd below 8363 becomes a negative finetune") {
      REQUIRE(module.samples[0].transpose == 0);
      REQUIRE(module.samples[0].finetune == -20);
    }
  }

  GIVEN("Data that is not an S3M module") {
    S3mModule module;
    THEN("It is rejected") {
      REQUIRE_FALSE(parseS3m(std::vector<uint8_t>(200, 0), module));
    }
  }
}

SCENARIO("The tracker steps through rows, ticks and orders") {
  GIVEN("The module playing from the start") {
    S3mModule module;
    REQUIRE(parseS3m(buildS3m(song()), module));
    Tracker tracker(module);

    WHEN("The first tick plays") {
      const TrackerTick &tick = tracker.advance();

      THEN("The first notes are triggered at their volumes and pans") {
        REQUIRE(tick.voices[0].trigger);
        REQUIRE(tick.voices[0].active);
        REQUIRE(tick.voices[0].gainLeft == 16 * 50 * 208 >> 8);
        REQUIRE(tick.voices[0].gainRight == 16 * 50 * 48 >> 8);
        REQUIRE(tick.voices[1].gainLeft == 16 * 40 * 64 >> 8);
        REQUIRE(tick.voices[1].gainRight == 16 * 40 * 192 >> 8);
        REQUIRE_FALSE(tick.voices[2].active);
      }

      THEN("C-4 of an 8363 Hz sample has the Amiga period 428") {
        REQUIRE(tick.voices[1].period ==
                Tracker::notePeriod(0x47 / 16 * 12 + 7 + 12, 0));
        REQUIRE(Tracker::notePeriod(60, 0) == 428u * Tracker::PERIOD_ONE);
      }
    }

    WHEN("Three ticks have played") {
      tracker.advance();
      tracker.advance();
      const TrackerTick third = tracker.advance();
      const TrackerTick fourth = tracker.advance();

      THEN("Each row lasts the speed's number of ticks") {
        REQUIRE(third.row == 0);
        REQUIRE(fourth.row == 1);
        REQUIRE_FALSE(fourth.voices[0].trigger);
      }
    }

    WHEN("The whole song has played") {
      int ticks = 0;
      while (tracker.loops() == 0) {
        tracker.advance();
        ++ticks;
      }

      THEN("Speed 4 from row 16 and the two breaks end it after 144 ticks") {
        REQUIRE(ticks == 16 * 3 + 9 * 4 + 15 * 4);
      }
    }
  }
}

SCENARIO("The tracker sounds like libxmp") {
  GIVEN("The module rendered by both players at 22050 Hz") {
    const std::vector<uint8_t> file = buildS3m(song());
    S3mModule module;
    REQUIRE(parseS3m(file, module));
    const std::vector<int16_t> ours = renderTracker(module);
    const std::vector<int16_t> theirs = renderXmp(file);

    THEN("Both last the same time and play the same waveform") {
      REQUIRE(ours.size() / 2 == Catch::Approx(theirs.size() / 2).margin(512));
      REQUIRE(correlation(ours, theirs) > 0.99);
    }
  }
}

namespace {

constexpr int REFERENCE_SEMITONES = 12;
constexpr int REFERENCE_FINE_STEPS = 128;
constexpr int REFERENCE_OCTAVE = REFERENCE_SEMITONES * REFERENCE_FINE_STEPS;
constexpr uint64_t REFERENCE_BASE = 13696ull * Tracker::PERIOD_ONE;
constexpr double REFERENCE_ONE = 1073741824.0;
constexpr int REFERENCE_FRACTION = 30;

uint64_t referenceRatio(int step, double divisor) {
  return static_cast<uint64_t>(
      std::floor(REFERENCE_ONE * std::pow(2.0, -step / divisor) + 0.5));
}

uint32_t referenceClamp(uint64_t period) {
  return static_cast<uint32_t>(std::clamp<uint64_t>(
      period, Tracker::PERIOD_ONE, 0xFFFFull * Tracker::PERIOD_ONE));
}

uint32_t referencePeriod(int note, int finetune) {
  int within = note * REFERENCE_FINE_STEPS + finetune;
  int octave = 0;
  while (within < 0) {
    within += REFERENCE_OCTAVE;
    --octave;
  }
  while (within >= REFERENCE_OCTAVE) {
    within -= REFERENCE_OCTAVE;
    ++octave;
  }
  uint64_t value = REFERENCE_BASE;
  value = value * referenceRatio(within / REFERENCE_FINE_STEPS,
                                 REFERENCE_SEMITONES) >>
          REFERENCE_FRACTION;
  value =
      value * referenceRatio(within % REFERENCE_FINE_STEPS, REFERENCE_OCTAVE) >>
      REFERENCE_FRACTION;
  value = octave >= 0 ? value >> octave : value << -octave;
  return referenceClamp(value);
}

} // namespace

SCENARIO("Tracker periods follow the equal tempered ratios of std::pow") {
  GIVEN("Every note with every finetune") {
    THEN("The period matches the one computed with std::pow") {
      for (int note = 0; note < 120; ++note) {
        for (int finetune = -127; finetune <= 127; ++finetune) {
          CAPTURE(note, finetune);
          REQUIRE(Tracker::notePeriod(note, finetune) ==
                  referencePeriod(note, finetune));
        }
      }
    }
  }

  GIVEN("Periods transposed upwards") {
    THEN("Each semitone matches the std::pow ratio") {
      const uint32_t periods[] = {Tracker::PERIOD_ONE,
                                  428 * Tracker::PERIOD_ONE, 12345,
                                  0xFFFF * Tracker::PERIOD_ONE};
      for (const uint32_t period : periods) {
        for (int semitones = 0; semitones <= 40; ++semitones) {
          CAPTURE(period, semitones);
          uint32_t expected = period;
          if (semitones > 0) {
            const uint64_t scaled =
                static_cast<uint64_t>(period) *
                    referenceRatio(semitones % REFERENCE_SEMITONES,
                                   REFERENCE_SEMITONES) >>
                REFERENCE_FRACTION;
            expected =
                referenceClamp(scaled >> (semitones / REFERENCE_SEMITONES));
          }
          REQUIRE(Tracker::transposed(period, semitones) == expected);
        }
      }
    }
  }
}

SCENARIO("S3M sample data becomes signed bytes") {
  GIVEN("A module with unsigned samples") {
    const ModuleSpec spec = song();
    const std::vector<uint8_t> file = buildS3m(spec);

    THEN("Every byte has its sign bit flipped") {
      S3mModule module;
      REQUIRE(parseS3m(file, module));
      for (std::size_t index = 0; index < spec.samples.size(); ++index) {
        CAPTURE(index);
        REQUIRE(module.samples[index].data ==
                converted(spec.samples[index].data, SIGN_BIT));
      }
    }

    THEN("A sign flip that does the work is used") {
      S3mModule module;
      REQUIRE(parseS3m(file, module, flipToConstant));
      REQUIRE(module.samples[0].data ==
              std::vector<int8_t>(spec.samples[0].data.size(), FLIPPED));
    }

    THEN("A sign flip that refuses leaves the work to the parser") {
      S3mModule module;
      REQUIRE(parseS3m(file, module, refuseFlip));
      REQUIRE(module.samples[0].data ==
              converted(spec.samples[0].data, SIGN_BIT));
    }
  }

  GIVEN("A module with signed samples") {
    const ModuleSpec spec = song();
    std::vector<uint8_t> file = buildS3m(spec);
    put16(file, 0x2A, 1);

    THEN("The bytes are kept and no sign flip is asked for") {
      S3mModule module;
      REQUIRE(parseS3m(file, module, flipToConstant));
      for (std::size_t index = 0; index < spec.samples.size(); ++index) {
        CAPTURE(index);
        REQUIRE(module.samples[index].data ==
                converted(spec.samples[index].data, 0));
      }
    }
  }
}

SCENARIO("An S3M module is read in steps") {
  GIVEN("A two-pattern module with two samples") {
    const std::vector<uint8_t> file = buildS3m(song());
    S3mModule whole;
    REQUIRE(parseS3m(file, whole));

    THEN("Each sample and pattern takes a step and the result is the same") {
      S3mReader reader(file, nullptr, 0);
      S3mModule stepped;
      int steps = 1;
      while (!reader.step(stepped)) {
        ++steps;
      }
      REQUIRE_FALSE(reader.failed());
      REQUIRE(steps == 1 + 2 + 2);
      REQUIRE(stepped.orders == whole.orders);
      REQUIRE(stepped.pans == whole.pans);
      REQUIRE(stepped.tempoRows == whole.tempoRows);
      REQUIRE(stepped.samples.size() == whole.samples.size());
      for (std::size_t index = 0; index < whole.samples.size(); ++index) {
        REQUIRE(stepped.samples[index].data == whole.samples[index].data);
        REQUIRE(stepped.samples[index].loopEnd == whole.samples[index].loopEnd);
      }
      REQUIRE(stepped.patterns.size() == whole.patterns.size());
      for (std::size_t index = 0; index < whole.patterns.size(); ++index) {
        for (std::size_t row = 0; row < S3mModule::ROWS; ++row) {
          for (std::size_t channel = 0; channel < S3mModule::CHANNELS;
               ++channel) {
            const S3mEvent &a = stepped.patterns[index][row][channel];
            const S3mEvent &b = whole.patterns[index][row][channel];
            REQUIRE(a.note == b.note);
            REQUIRE(a.instrument == b.instrument);
            REQUIRE(a.volume == b.volume);
            REQUIRE(a.command == b.command);
            REQUIRE(a.parameter == b.parameter);
          }
        }
      }
    }

    THEN("A budget of steps is kept") {
      S3mReader reader(file, nullptr, 3);
      S3mModule stepped;
      int steps = 1;
      while (!reader.step(stepped)) {
        ++steps;
      }
      REQUIRE(steps <= 3);
      REQUIRE(stepped.patterns.size() == 2);
    }
  }

  GIVEN("Data that is not a module") {
    THEN("The reader stops at once and reports it") {
      S3mReader reader(std::vector<uint8_t>(200, 0), nullptr, 0);
      S3mModule module;
      REQUIRE(reader.step(module));
      REQUIRE(reader.failed());
    }
  }
}

namespace {

constexpr uint8_t SLIDE_DOWN = 5;
constexpr uint8_t SLIDE_UP = 6;
constexpr uint8_t JUMP = 2;

ModuleSpec slides() {
  ModuleSpec spec;
  spec.speed = 2;
  spec.orders = {0, 1, 255};
  SampleSpec tone;
  tone.data = wave(2000, 40, 90);
  spec.samples = {tone};
  spec.patterns = {
      {
          {0, 0, C4, 1, -1, 0, 0},
          {1, 0, NO_NOTE, 0, -1, SLIDE_DOWN, 0xF3},
          {2, 0, NO_NOTE, 0, -1, SLIDE_UP, 0xE4},
          {3, 0, NO_NOTE, 0, -1, SLIDE_UP, 0},
      },
      {
          {0, 0, NO_NOTE, 0, -1, JUMP, 0},
      },
  };
  return spec;
}

std::vector<TrackerTick> ticks(Tracker &tracker, int count) {
  std::vector<TrackerTick> played;
  for (int tick = 0; tick < count; ++tick) {
    played.push_back(tracker.advance());
  }
  return played;
}

std::size_t pointerAt(const ModuleSpec &spec, std::size_t index) {
  return 0x60 + spec.orders.size() + 2 * index;
}

} // namespace

SCENARIO("Fine slides move the period once, on the row's first tick") {
  GIVEN("C-4 followed by a fine slide down and two extra fine slides up") {
    S3mModule module;
    REQUIRE(parseS3m(buildS3m(slides()), module));
    Tracker tracker(module);
    const std::vector<TrackerTick> played = ticks(tracker, 8);
    const uint32_t c4 = 428 * Tracker::PERIOD_ONE;

    THEN("EF3 lowers the note by three periods for both ticks of its row") {
      REQUIRE(played[0].voices[0].period == c4);
      REQUIRE(played[2].voices[0].period == c4 + 3 * Tracker::PERIOD_ONE);
      REQUIRE(played[3].voices[0].period == c4 + 3 * Tracker::PERIOD_ONE);
    }

    THEN("FE4 raises it by one period, and F00 repeats the last slide") {
      REQUIRE(played[4].voices[0].period == c4 + 2 * Tracker::PERIOD_ONE);
      REQUIRE(played[5].voices[0].period == c4 + 2 * Tracker::PERIOD_ONE);
      REQUIRE(played[6].voices[0].period == c4 + Tracker::PERIOD_ONE);
      REQUIRE(played[7].voices[0].period == c4 + Tracker::PERIOD_ONE);
    }
  }
}

SCENARIO("A pattern plays its 64 rows, then a position jump goes back") {
  GIVEN("A first pattern without breaks and a second that jumps to order 0") {
    S3mModule module;
    REQUIRE(parseS3m(buildS3m(slides()), module));
    Tracker tracker(module);

    WHEN("All 64 rows of the first pattern have played") {
      const std::vector<TrackerTick> played = ticks(tracker, 64 * 2 + 1);

      THEN("The second order starts on its first row") {
        REQUIRE(played[127].pattern == 0);
        REQUIRE(played[127].row == 63);
        REQUIRE(played[128].pattern == 1);
        REQUIRE(played[128].row == 0);
        REQUIRE(tracker.loops() == 0);
      }
    }

    WHEN("The jump's row is over") {
      ticks(tracker, 64 * 2 + 1);
      const bool loopedEarly = tracker.loops() != 0;
      tracker.advance();
      const int loops = tracker.loops();
      const TrackerTick next = tracker.advance();

      THEN("Jumping back to the first order counts as a loop") {
        REQUIRE_FALSE(loopedEarly);
        REQUIRE(loops == 1);
        REQUIRE(next.restarted);
        REQUIRE(next.pattern == 0);
        REQUIRE(next.row == 0);
      }
    }
  }

  GIVEN("A jump from the first order over the second to the third") {
    ModuleSpec spec = slides();
    spec.orders = {0, 1, 2, 255};
    spec.patterns[0] = {{0, 0, C4, 1, -1, JUMP, 2}};
    spec.patterns.push_back({{0, 0, C4, 1, -1, 0, 0}});
    S3mModule module;
    REQUIRE(parseS3m(buildS3m(spec), module));
    Tracker tracker(module);
    const std::vector<TrackerTick> played = ticks(tracker, 3);

    THEN("The third order plays next and no loop is counted") {
      REQUIRE(played[1].pattern == 0);
      REQUIRE(played[2].pattern == 2);
      REQUIRE(played[2].row == 0);
      REQUIRE_FALSE(played[2].restarted);
      REQUIRE(tracker.loops() == 0);
    }
  }
}

SCENARIO("A module with nothing to play stays silent") {
  GIVEN("Order lists that end, or skip, before any pattern") {
    const std::vector<std::vector<uint8_t>> orders = {
        {255, 0}, {254, 255, 0}, {7}, {}};

    THEN("The tracker is empty and every tick is silent at the module's "
         "speed") {
      for (const std::vector<uint8_t> &list : orders) {
        ModuleSpec spec = slides();
        spec.orders = list;
        S3mModule module;
        REQUIRE(parseS3m(buildS3m(spec), module));
        Tracker tracker(module);
        REQUIRE(tracker.isEmpty());
        for (int tick = 0; tick < 3; ++tick) {
          const TrackerTick &played = tracker.advance();
          REQUIRE(played.speed == 2);
          REQUIRE(played.bpm == 125);
          for (const TrackerVoice &voice : played.voices) {
            REQUIRE_FALSE(voice.active);
            REQUIRE_FALSE(voice.trigger);
          }
        }
        REQUIRE(tracker.loops() == 0);
      }
    }
  }

  GIVEN("A module that plays") {
    S3mModule module;
    REQUIRE(parseS3m(buildS3m(slides()), module));

    THEN("It is not empty") { REQUIRE_FALSE(Tracker(module).isEmpty()); }
  }
}

SCENARIO("S3M parts the tracker cannot play are left out") {
  GIVEN("An AdLib instrument in place of the first sample") {
    const ModuleSpec spec = slides();
    std::vector<uint8_t> file = buildS3m(spec);
    const std::size_t pointer = pointerAt(spec, 0);
    file[(file[pointer] | file[pointer + 1] << 8) * 16u] = 2;
    S3mModule module;
    REQUIRE(parseS3m(file, module));

    THEN("It has no data, so its note does not sound") {
      REQUIRE(module.samples[0].data.empty());
      REQUIRE(module.samples[0].volume == 0);
      Tracker tracker(module);
      REQUIRE_FALSE(tracker.advance().voices[0].active);
    }
  }

  GIVEN("A pattern whose pointer is 0") {
    const ModuleSpec spec = slides();
    std::vector<uint8_t> file = buildS3m(spec);
    const std::size_t pointer = pointerAt(spec, spec.samples.size());
    file[pointer] = 0;
    file[pointer + 1] = 0;
    S3mModule module;
    REQUIRE(parseS3m(file, module));

    THEN("The pattern is there but empty") {
      REQUIRE(module.patterns.size() == 2);
      for (const S3mModule::Row &row : module.patterns[0]) {
        for (const S3mEvent &event : row) {
          REQUIRE(event.note == 0);
          REQUIRE(event.instrument == 0);
          REQUIRE(event.command == 0);
        }
      }
      REQUIRE(module.patterns[1][0][0].command == JUMP);
    }
  }

  GIVEN("Events on channels past the fourth, between events on the first "
        "two") {
    ModuleSpec spec = slides();
    spec.patterns[0] = {{0, 0, C4, 1, 20, 0, 0},
                        {0, 5, 0x47, 1, 30, SLIDE_DOWN, 0x22},
                        {0, 1, 0x42, 1, 40, SLIDE_UP, 0x11},
                        {1, 9, 0x30, 1, -1, 0, 0},
                        {1, 2, 0x31, 1, -1, 0, 0}};
    S3mModule module;
    REQUIRE(parseS3m(buildS3m(spec), module));
    const S3mModule::Pattern &pattern = module.patterns[0];

    THEN("They are skipped and the events after them are read whole") {
      REQUIRE(pattern[0][0].note == 13 + 12 * 4);
      REQUIRE(pattern[0][0].volume == 21);
      REQUIRE(pattern[0][0].command == 0);
      REQUIRE(pattern[0][1].note == 13 + 12 * 4 + 2);
      REQUIRE(pattern[0][1].volume == 41);
      REQUIRE(pattern[0][1].command == SLIDE_UP);
      REQUIRE(pattern[0][1].parameter == 0x11);
      REQUIRE(pattern[0][2].note == 0);
      REQUIRE(pattern[0][3].note == 0);
      REQUIRE(pattern[1][2].note == 13 + 12 * 3 + 1);
      REQUIRE(pattern[1][0].note == 0);
    }
  }
}

SCENARIO("A reader with a budget of one or two steps reads all at once") {
  GIVEN("The two-pattern module") {
    const std::vector<uint8_t> file = buildS3m(song());
    S3mModule whole;
    REQUIRE(parseS3m(file, whole));

    THEN("The header takes a step and everything else the next") {
      for (const int budget : {1, 2}) {
        S3mReader reader(file, nullptr, budget);
        S3mModule stepped;
        int steps = 1;
        while (!reader.step(stepped)) {
          ++steps;
        }
        REQUIRE(steps == 2);
        REQUIRE(stepped.patterns.size() == whole.patterns.size());
        REQUIRE(stepped.samples.size() == whole.samples.size());
        REQUIRE(stepped.samples[1].data == whole.samples[1].data);
        REQUIRE(stepped.tempoRows == whole.tempoRows);
      }
    }
  }
}
