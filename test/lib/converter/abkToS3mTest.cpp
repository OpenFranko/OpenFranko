#include "../../../lib/converter/abkToS3m/abkToS3m.h"

#include "../../../lib/binary/binary.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace openfranko::lib::converter::abkToS3m;
using namespace openfranko::lib::binary;

namespace {

std::vector<uint8_t> buildMinimalAbk(const char *songName = "test song",
                                     uint16_t amosTempo = 17) {
  std::vector<uint8_t> music;

  std::size_t sampleInfoOff = 12;
  std::size_t songOff = 12 + 36;
  std::size_t trackOff = songOff + 6 + 28 + 6;

  pushBigEndian32(music, static_cast<uint32_t>(sampleInfoOff));
  pushBigEndian32(music, static_cast<uint32_t>(songOff));
  pushBigEndian32(music, static_cast<uint32_t>(trackOff));

  pushBigEndian16(music, 1);
  pushBigEndian32(music, 34);
  pushBigEndian32(music, 34);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 1);
  pushBigEndian16(music, 63);
  pushBigEndian16(music, 1);
  char sampleName[16] = "testsample";
  music.insert(music.end(), sampleName, sampleName + 16);
  music.push_back(0x40);
  music.push_back(0xC0);

  pushBigEndian16(music, 0);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 6);
  pushBigEndian16(music, 20);
  pushBigEndian16(music, 22);
  pushBigEndian16(music, 24);
  pushBigEndian16(music, 26);
  pushBigEndian16(music, amosTempo);
  pushBigEndian16(music, 0);
  char nameBuf[16] = {};
  std::strncpy(nameBuf, songName, 15);
  music.insert(music.end(), nameBuf, nameBuf + 16);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 0xFFFF);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 0xFFFF);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 0xFFFF);
  pushBigEndian16(music, 0);
  pushBigEndian16(music, 0xFFFF);

  std::size_t expectedTrackOff = trackOff;
  while (music.size() < expectedTrackOff) {
    music.push_back(0);
  }

  pushBigEndian16(music, 1);
  uint16_t patternDataOff = static_cast<uint16_t>(2 + 4 * 2);
  pushBigEndian16(music, patternDataOff);
  pushBigEndian16(music, patternDataOff);
  pushBigEndian16(music, patternDataOff);
  pushBigEndian16(music, patternDataOff);
  music.push_back(0x80);
  music.push_back(0x00);

  std::vector<uint8_t> abk;
  abk.push_back('A');
  abk.push_back('m');
  abk.push_back('B');
  abk.push_back('k');
  pushBigEndian16(abk, 3);
  pushBigEndian16(abk, 0);
  pushBigEndian32(abk, static_cast<uint32_t>(music.size() + 8) | 0x80000000u);
  const char *bankName = "Music   ";
  abk.insert(abk.end(), bankName, bankName + 8);
  abk.insert(abk.end(), music.begin(), music.end());

  return abk;
}

constexpr int CHANNELS = 4;
constexpr int ROWS = 64;

constexpr uint16_t END = 0x8000;
constexpr uint16_t UNTRANSLATED_COMMAND = 0x8100;
constexpr uint16_t SET_VOLUME = 0x8300;
constexpr uint16_t STOP_EFFECT = 0x8400;
constexpr uint16_t SET_TEMPO = 0x8800;
constexpr uint16_t SET_SAMPLE = 0x8900;
constexpr uint16_t ARPEGGIO = 0x8A00;
constexpr uint16_t TONE_PORTAMENTO = 0x8B00;
constexpr uint16_t VIBRATO = 0x8C00;
constexpr uint16_t VOLUME_SLIDE = 0x8D00;
constexpr uint16_t SLIDE_UP = 0x8E00;
constexpr uint16_t SLIDE_DOWN = 0x8F00;
constexpr uint16_t DELAY = 0x9000;
constexpr uint16_t POSITION_JUMP = 0x9100;
constexpr uint16_t ORDER_LIST_END = 0xFFFF;

constexpr uint16_t PERIOD_C2 = 1712;
constexpr uint16_t PERIOD_C3 = 856;
constexpr uint16_t PERIOD_C4 = 428;
constexpr uint16_t PERIOD_NEAR_C4 = 430;
constexpr uint16_t PERIOD_C_SHARP_4 = 404;
constexpr uint16_t PERIOD_C5 = 214;
constexpr uint16_t PERIOD_FLAG_BITS = 0x4000;

constexpr uint8_t S3M_C2 = 0x20;
constexpr uint8_t S3M_C3 = 0x30;
constexpr uint8_t S3M_C4 = 0x40;
constexpr uint8_t S3M_C_SHARP_4 = 0x41;
constexpr uint8_t S3M_C5 = 0x50;
constexpr uint8_t NO_NOTE = 0xFF;
constexpr uint8_t NOTE_OFF = 0xFE;
constexpr uint8_t NO_VOLUME = 0xFF;
constexpr uint8_t ORDER_END_MARKER = 0xFF;

constexpr uint8_t EFFECT_SPEED = 1;
constexpr uint8_t EFFECT_POSITION_JUMP = 2;
constexpr uint8_t EFFECT_PATTERN_BREAK = 3;
constexpr uint8_t EFFECT_VOLUME_SLIDE = 4;
constexpr uint8_t EFFECT_PORTA_DOWN = 5;
constexpr uint8_t EFFECT_PORTA_UP = 6;
constexpr uint8_t EFFECT_TONE_PORTA = 7;
constexpr uint8_t EFFECT_VIBRATO = 8;
constexpr uint8_t EFFECT_ARPEGGIO = 10;
constexpr uint8_t EFFECT_TEMPO = 20;

constexpr std::size_t AMBK_HEADER_SIZE = 20;
constexpr std::size_t SAMPLES_POINTER = 0;
constexpr std::size_t SONGS_POINTER = 4;
constexpr std::size_t TRACKS_POINTER = 8;
constexpr std::size_t MUSIC_HEADER_SIZE = 12;
constexpr std::size_t NAME_SIZE = 16;
constexpr std::size_t SONG_HEADER_SIZE = 28;
constexpr std::size_t SAMPLE_DESCRIPTOR_SIZE = 32;
constexpr std::size_t PARAGRAPH = 16;
constexpr uint32_t AMIGA_SAMPLE_RATE = 8287;

using ChannelData = std::vector<uint16_t>;
using Step = std::array<ChannelData, CHANNELS>;

struct BankSample {
  std::string name;
  std::vector<int8_t> pcm;
  uint16_t volume = 64;
  uint32_t loopStart = 0;
  uint16_t loopWords = 0;
  std::optional<uint16_t> declaredWords = std::nullopt;
};

struct MusicBank {
  std::string songName = "song";
  uint16_t songSpeed = 17;
  std::array<std::vector<uint16_t>, CHANNELS> orders;
  std::vector<Step> steps;
  std::vector<BankSample> samples;
};

void pushName(std::vector<uint8_t> &out, const std::string &name) {
  std::array<uint8_t, NAME_SIZE> field{};
  std::copy_n(name.begin(), std::min(name.size(), field.size()), field.begin());
  out.insert(out.end(), field.begin(), field.end());
}

std::vector<uint8_t> songSection(const MusicBank &bank) {
  std::vector<uint8_t> section;
  pushBigEndian16(section, 1);
  pushBigEndian32(section, 6);
  std::vector<uint8_t> orderLists;
  std::array<uint16_t, CHANNELS> listOffsets{};
  for (int channel = 0; channel < CHANNELS; ++channel) {
    listOffsets[channel] =
        static_cast<uint16_t>(SONG_HEADER_SIZE + orderLists.size());
    for (uint16_t step : bank.orders[channel]) {
      pushBigEndian16(orderLists, step);
    }
    pushBigEndian16(orderLists, ORDER_LIST_END);
  }
  for (uint16_t offset : listOffsets) {
    pushBigEndian16(section, offset);
  }
  pushBigEndian16(section, bank.songSpeed);
  pushBigEndian16(section, 0);
  pushName(section, bank.songName);
  section.insert(section.end(), orderLists.begin(), orderLists.end());
  return section;
}

std::vector<uint8_t> trackSection(const MusicBank &bank) {
  std::vector<uint8_t> section;
  pushBigEndian16(section, static_cast<uint16_t>(bank.steps.size()));
  const std::size_t tableEnd = 2 + bank.steps.size() * CHANNELS * 2;
  std::vector<uint8_t> words;
  for (const Step &step : bank.steps) {
    for (const ChannelData &channel : step) {
      if (channel.empty()) {
        pushBigEndian16(section, 0);
        continue;
      }
      pushBigEndian16(section, static_cast<uint16_t>(tableEnd + words.size()));
      for (uint16_t word : channel) {
        pushBigEndian16(words, word);
      }
    }
  }
  section.insert(section.end(), words.begin(), words.end());
  return section;
}

std::vector<uint8_t> sampleSection(const MusicBank &bank) {
  std::vector<uint8_t> section;
  pushBigEndian16(section, static_cast<uint16_t>(bank.samples.size()));
  const std::size_t pcmStart = 2 + bank.samples.size() * SAMPLE_DESCRIPTOR_SIZE;
  std::vector<uint8_t> pcm;
  for (const BankSample &sample : bank.samples) {
    const auto start = static_cast<uint32_t>(pcmStart + pcm.size());
    pushBigEndian32(section, start);
    pushBigEndian32(section, start + sample.loopStart);
    pushBigEndian16(section, 0);
    pushBigEndian16(section, sample.loopWords);
    pushBigEndian16(section, sample.volume);
    pushBigEndian16(section, sample.declaredWords.value_or(
                                 static_cast<uint16_t>(sample.pcm.size() / 2)));
    pushName(section, sample.name);
    for (int8_t value : sample.pcm) {
      pcm.push_back(static_cast<uint8_t>(value));
    }
  }
  section.insert(section.end(), pcm.begin(), pcm.end());
  return section;
}

std::vector<uint8_t> wrapInAmBk(const std::vector<uint8_t> &music) {
  std::vector<uint8_t> abk = {'A', 'm', 'B', 'k'};
  pushBigEndian16(abk, 3);
  pushBigEndian16(abk, 0);
  pushBigEndian32(abk, static_cast<uint32_t>(music.size() + 8) | 0x80000000u);
  const std::string bankName = "Music   ";
  abk.insert(abk.end(), bankName.begin(), bankName.end());
  abk.insert(abk.end(), music.begin(), music.end());
  return abk;
}

std::vector<uint8_t> build(const MusicBank &bank) {
  const std::vector<uint8_t> song = songSection(bank);
  const std::vector<uint8_t> tracks = trackSection(bank);
  const std::vector<uint8_t> samples = sampleSection(bank);
  const auto songOffset = static_cast<uint32_t>(MUSIC_HEADER_SIZE);
  const auto trackOffset = static_cast<uint32_t>(songOffset + song.size());
  const auto sampleOffset = static_cast<uint32_t>(trackOffset + tracks.size());
  std::vector<uint8_t> music;
  pushBigEndian32(music, sampleOffset);
  pushBigEndian32(music, songOffset);
  pushBigEndian32(music, trackOffset);
  music.insert(music.end(), song.begin(), song.end());
  music.insert(music.end(), tracks.begin(), tracks.end());
  music.insert(music.end(), samples.begin(), samples.end());
  return wrapInAmBk(music);
}

std::size_t sectionAt(const std::vector<uint8_t> &abk, std::size_t pointer) {
  return AMBK_HEADER_SIZE +
         BigEndianReader(abk).readUint32(AMBK_HEADER_SIZE + pointer);
}

void patchWord(std::vector<uint8_t> &data, std::size_t at, uint16_t value) {
  data.at(at) = static_cast<uint8_t>(value >> 8);
  data.at(at + 1) = static_cast<uint8_t>(value);
}

void patchLong(std::vector<uint8_t> &data, std::size_t at, uint32_t value) {
  patchWord(data, at, static_cast<uint16_t>(value >> 16));
  patchWord(data, at + 2, static_cast<uint16_t>(value));
}

struct Cell {
  uint8_t note = NO_NOTE;
  uint8_t instrument = 0;
  uint8_t volume = NO_VOLUME;
  uint8_t effect = 0;
  uint8_t parameter = 0;

  bool operator==(const Cell &other) const {
    return note == other.note && instrument == other.instrument &&
           volume == other.volume && effect == other.effect &&
           parameter == other.parameter;
  }
};

std::ostream &operator<<(std::ostream &out, const Cell &cell) {
  return out << "{note " << int{cell.note} << ", instrument "
             << int{cell.instrument} << ", volume " << int{cell.volume}
             << ", effect " << int{cell.effect} << ", parameter "
             << int{cell.parameter} << "}";
}

Cell effect(uint8_t command, uint8_t parameter) {
  return {NO_NOTE, 0, NO_VOLUME, command, parameter};
}

using Rows = std::array<std::array<Cell, CHANNELS>, ROWS>;

struct Instrument {
  uint8_t type = 0;
  std::string dosName;
  uint32_t length = 0;
  uint32_t loopStart = 0;
  uint32_t loopEnd = 0;
  uint8_t volume = 0;
  uint8_t flags = 0;
  uint32_t c2spd = 0;
  std::string name;
  std::string signature;
  std::vector<uint8_t> data;
};

struct Module {
  std::string name;
  uint16_t orderCount = 0;
  uint16_t instrumentCount = 0;
  uint16_t patternCount = 0;
  uint8_t speed = 0;
  uint8_t tempo = 0;
  std::vector<uint8_t> orders;
  std::vector<uint8_t> panning;
  std::vector<Instrument> instruments;
  std::vector<Rows> patterns;
  std::vector<std::size_t> packedSizes;
  std::vector<std::size_t> bytesRead;
};

std::vector<uint8_t> bytesAt(const std::vector<uint8_t> &data, std::size_t at,
                             std::size_t size) {
  if (at > data.size() || size > data.size() - at) {
    throw std::out_of_range("S3M field past the end of the file");
  }
  return {data.begin() + static_cast<std::ptrdiff_t>(at),
          data.begin() + static_cast<std::ptrdiff_t>(at + size)};
}

std::string textAt(const std::vector<uint8_t> &data, std::size_t at,
                   std::size_t size) {
  const std::vector<uint8_t> field = bytesAt(data, at, size);
  const auto end = std::find(field.begin(), field.end(), 0);
  return std::string(field.begin(), end);
}

Instrument readInstrument(const std::vector<uint8_t> &s3m, std::size_t at) {
  const LittleEndianReader reader(s3m);
  Instrument instrument;
  instrument.type = s3m.at(at);
  instrument.dosName = textAt(s3m, at + 0x01, 12);
  const std::size_t dataParagraph = static_cast<std::size_t>(s3m.at(at + 0x0D))
                                        << 16 |
                                    reader.readUint16(at + 0x0E);
  instrument.length = reader.readUint32(at + 0x10);
  instrument.loopStart = reader.readUint32(at + 0x14);
  instrument.loopEnd = reader.readUint32(at + 0x18);
  instrument.volume = s3m.at(at + 0x1C);
  instrument.flags = s3m.at(at + 0x1F);
  instrument.c2spd = reader.readUint32(at + 0x20);
  instrument.name = textAt(s3m, at + 0x30, 28);
  instrument.signature = textAt(s3m, at + 0x4C, 4);
  instrument.data = bytesAt(s3m, dataParagraph * PARAGRAPH, instrument.length);
  return instrument;
}

Rows readPattern(const std::vector<uint8_t> &s3m, std::size_t at,
                 std::size_t &packedSize, std::size_t &bytesRead) {
  Rows rows{};
  packedSize = LittleEndianReader(s3m).readUint16(at);
  std::size_t pos = at + 2;
  for (auto &row : rows) {
    for (uint8_t what = s3m.at(pos++); what != 0; what = s3m.at(pos++)) {
      Cell &cell = row.at(what & 0x1F);
      if (what & 0x20) {
        cell.note = s3m.at(pos++);
        cell.instrument = s3m.at(pos++);
      }
      if (what & 0x40) {
        cell.volume = s3m.at(pos++);
      }
      if (what & 0x80) {
        cell.effect = s3m.at(pos++);
        cell.parameter = s3m.at(pos++);
      }
    }
  }
  bytesRead = pos - at;
  return rows;
}

Module readModule(const std::vector<uint8_t> &s3m) {
  const LittleEndianReader reader(s3m);
  Module module;
  module.name = textAt(s3m, 0, 28);
  module.orderCount = reader.readUint16(0x20);
  module.instrumentCount = reader.readUint16(0x22);
  module.patternCount = reader.readUint16(0x24);
  module.speed = s3m.at(0x31);
  module.tempo = s3m.at(0x32);
  module.orders = bytesAt(s3m, 0x60, module.orderCount);
  const std::size_t instrumentPointers = 0x60 + module.orderCount;
  const std::size_t patternPointers =
      instrumentPointers + 2 * std::size_t{module.instrumentCount};
  const std::size_t panningAt =
      patternPointers + 2 * std::size_t{module.patternCount};
  module.panning = bytesAt(s3m, panningAt, 32);
  for (std::size_t i = 0; i < module.instrumentCount; ++i) {
    module.instruments.push_back(readInstrument(
        s3m, reader.readUint16(instrumentPointers + 2 * i) * PARAGRAPH));
  }
  for (std::size_t i = 0; i < module.patternCount; ++i) {
    std::size_t packedSize = 0;
    std::size_t bytesRead = 0;
    module.patterns.push_back(
        readPattern(s3m, reader.readUint16(patternPointers + 2 * i) * PARAGRAPH,
                    packedSize, bytesRead));
    module.packedSizes.push_back(packedSize);
    module.bytesRead.push_back(bytesRead);
  }
  return module;
}

bool rowsEmpty(const Rows &rows, int firstRow, int channel) {
  for (int row = firstRow; row < ROWS; ++row) {
    if (!(rows[row][channel] == Cell{})) {
      return false;
    }
  }
  return true;
}

MusicBank songOfOnePattern(const Step &step) {
  MusicBank bank;
  bank.steps = {step};
  bank.orders[0] = {0};
  return bank;
}

MusicBank songWithDistinctPatterns(int count) {
  MusicBank bank;
  for (int i = 0; i < count; ++i) {
    bank.steps.push_back(
        Step{ChannelData{static_cast<uint16_t>(SET_VOLUME | (i % 64)),
                         static_cast<uint16_t>(DELAY | (i / 64 + 1)), END}});
    bank.orders[0].push_back(static_cast<uint16_t>(i));
  }
  return bank;
}

} // namespace

SCENARIO("convert rejects invalid input") {
  GIVEN("A buffer that is too small") {
    std::vector<uint8_t> data = {0x01, 0x02};

    THEN("It throws") { REQUIRE_THROWS_AS(convert(data), std::runtime_error); }
  }

  GIVEN("A buffer without AmBk magic") {
    std::vector<uint8_t> data(30, 0);
    data[0] = 'X';

    THEN("It throws") { REQUIRE_THROWS_AS(convert(data), std::runtime_error); }
  }

  GIVEN("Banks whose magic differs from AmBk in one letter") {
    THEN("Each is refused") {
      for (std::size_t letter = 0; letter < 4; ++letter) {
        auto data = build(songOfOnePattern(Step{ChannelData{DELAY | 64}}));
        data[letter] = '?';
        CAPTURE(letter);
        REQUIRE_THROWS_WITH(convert(data), "Not a valid AmBk file");
      }
    }
  }

  GIVEN("An AmBk bank whose music data is shorter than its 12-byte header") {
    const auto data = wrapInAmBk(std::vector<uint8_t>(11, 0));

    THEN("It throws") {
      REQUIRE_THROWS_WITH(convert(data), "Music data too small");
    }
  }

  GIVEN("A song without any orders") {
    MusicBank bank;
    bank.steps = {Step{ChannelData{PERIOD_C4, END}}};

    THEN("It throws") {
      REQUIRE_THROWS_WITH(convert(build(bank)), "Empty song");
    }
  }
}

SCENARIO("convert produces a valid S3M file") {
  GIVEN("A minimal valid ABK file") {
    auto abk = buildMinimalAbk();

    WHEN("convert is called") {
      auto s3m = convert(abk);

      THEN("Output has S3M signature at offset 0x2C") {
        REQUIRE(s3m.size() > 0x30);
        REQUIRE(s3m[0x2C] == 'S');
        REQUIRE(s3m[0x2D] == 'C');
        REQUIRE(s3m[0x2E] == 'R');
        REQUIRE(s3m[0x2F] == 'M');
      }

      THEN("Output has EOF marker at 0x1C") { REQUIRE(s3m[0x1C] == 0x1A); }

      THEN("Song name is embedded in the header") {
        std::string name(reinterpret_cast<const char *>(s3m.data()), 9);
        REQUIRE(name == "test song");
      }

      THEN("Order count is at least 2 (padded to even)") {
        LittleEndianReader reader(s3m);
        uint16_t ordNum = reader.readUint16(0x20);
        REQUIRE(ordNum >= 2);
        REQUIRE(ordNum % 2 == 0);
      }

      THEN("Instrument count is 1") {
        REQUIRE(LittleEndianReader(s3m).readUint16(0x22) == 1);
      }

      THEN("Pattern count is at least 1") {
        REQUIRE(LittleEndianReader(s3m).readUint16(0x24) >= 1);
      }

      THEN("Global volume is 64") { REQUIRE(s3m[0x30] == 64); }
    }
  }
}

SCENARIO("convert uses the Franko menu tempo for song 'e1'") {
  GIVEN("An ABK with song name 'e1' and AMOS tempo 33") {
    auto abk = buildMinimalAbk("e1", 33);

    WHEN("convert is called") {
      auto s3m = convert(abk);

      THEN("Speed = round(100/37) = 3") { REQUIRE(s3m[0x31] == 3); }

      THEN("BPM = round(5*3*37/4) = 139, not 124 from tempo 33") {
        REQUIRE(s3m[0x32] == 139);
      }
    }
  }
}

SCENARIO("convert maps AMOS tempo to S3M speed/tempo") {
  GIVEN("An ABK with AMOS tempo 25") {
    auto abk = buildMinimalAbk("level1", 25);

    WHEN("convert is called") {
      auto s3m = convert(abk);

      THEN("Speed = round(100/25) = 4") { REQUIRE(s3m[0x31] == 4); }

      THEN("BPM = 5*4*25/4 = 125") { REQUIRE(s3m[0x32] == 125); }
    }
  }

  GIVEN("An ABK with AMOS tempo 17 (default)") {
    auto abk = buildMinimalAbk("default", 17);

    WHEN("convert is called") {
      auto s3m = convert(abk);

      THEN("Speed = round(100/17) = 6") { REQUIRE(s3m[0x31] == 6); }
    }
  }

  GIVEN("A song whose AMOS tempo is 0") {
    MusicBank bank = songOfOnePattern(Step{ChannelData{DELAY | 64}});
    bank.songSpeed = 0;
    const Module module = readModule(convert(build(bank)));

    THEN("The S3M starts at the default speed 6 and tempo 134") {
      REQUIRE(module.speed == 6);
      REQUIRE(module.tempo == 134);
    }
  }

  GIVEN("A song at AMOS tempo 1") {
    MusicBank bank = songOfOnePattern(Step{ChannelData{DELAY | 64}});
    bank.songSpeed = 1;
    const Module module = readModule(convert(build(bank)));

    THEN("The speed is capped at 31 and the tempo follows it") {
      REQUIRE(module.speed == 31);
      REQUIRE(module.tempo == 39);
    }
  }

  GIVEN("A song at AMOS tempo 17 converted with an initial tempo") {
    const auto abk = build(songOfOnePattern(Step{ChannelData{DELAY | 64}}));

    WHEN("The initial tempo is 40") {
      const Module module = readModule(convert(abk, 40));

      THEN("It replaces the song's own tempo") {
        REQUIRE(module.speed == 3);
        REQUIRE(module.tempo == 150);
      }
    }

    WHEN("The initial tempo is 300") {
      const Module module = readModule(convert(abk, 300));

      THEN("It is treated as 255, the fastest AMOS tempo") {
        REQUIRE(module.speed == 1);
        REQUIRE(module.tempo == 255);
      }
    }
  }
}

SCENARIO("convert writes the song's name, counts and channel layout into the "
         "S3M header") {
  GIVEN("A song called Theme at AMOS tempo 25 with two samples and two "
        "patterns played in three positions") {
    MusicBank bank;
    bank.songName = "Theme";
    bank.songSpeed = 25;
    bank.samples = {{"kick", {1, 2}}, {"snare", {3, 4}}};
    bank.steps = {Step{ChannelData{PERIOD_C4, DELAY | 64}},
                  Step{ChannelData{PERIOD_C5, DELAY | 64}}};
    bank.orders[0] = {0, 1, 0};
    const std::vector<uint8_t> s3m = convert(build(bank));
    const Module module = readModule(s3m);

    THEN("The header names the song and counts its orders, instruments and "
         "patterns") {
      REQUIRE(module.name == "Theme");
      REQUIRE(module.orderCount == 4);
      REQUIRE(module.instrumentCount == 2);
      REQUIRE(module.patternCount == 2);
    }

    THEN("The odd order list is padded with an end marker") {
      REQUIRE(module.orders == std::vector<uint8_t>{0, 1, 0, ORDER_END_MARKER});
    }

    THEN("Speed and tempo come from the song's AMOS tempo") {
      REQUIRE(module.speed == 4);
      REQUIRE(module.tempo == 125);
    }

    THEN("It is a stereo Scream Tracker 3.20 module with unsigned samples") {
      const LittleEndianReader reader(s3m);
      REQUIRE(s3m[0x1C] == 0x1A);
      REQUIRE(s3m[0x1D] == 0x10);
      REQUIRE(reader.readUint16(0x28) == 0x1320);
      REQUIRE(s3m[0x2A] == 2);
      REQUIRE(textAt(s3m, 0x2C, 4) == "SCRM");
      REQUIRE(s3m[0x30] == 64);
      REQUIRE(s3m[0x33] == (0x80 | 48));
      REQUIRE(s3m[0x34] == 16);
      REQUIRE(s3m[0x35] == 0xFC);
    }

    THEN("Four channels are laid out left, right, right, left like the "
         "Amiga's") {
      REQUIRE(bytesAt(s3m, 0x40, 4) == std::vector<uint8_t>{0, 8, 9, 1});
      REQUIRE(bytesAt(s3m, 0x44, 28) == std::vector<uint8_t>(28, 0xFF));
      std::vector<uint8_t> panning(32, 0);
      panning[0] = 0x23;
      panning[1] = 0x2C;
      panning[2] = 0x2C;
      panning[3] = 0x23;
      REQUIRE(module.panning == panning);
    }

    THEN("Each pattern's stored length matches its packed rows") {
      REQUIRE(module.packedSizes == module.bytesRead);
    }
  }
}

SCENARIO("convert stores each distinct pattern once") {
  GIVEN("Orders 0, 1, 2, 0, 1 where step 2 holds the same notes as step 0") {
    MusicBank bank;
    bank.steps = {Step{ChannelData{PERIOD_C4, DELAY | 64}},
                  Step{ChannelData{PERIOD_C5, DELAY | 64}},
                  Step{ChannelData{PERIOD_C4, DELAY | 64}}};
    bank.orders[0] = {0, 1, 2, 0, 1};
    const Module module = readModule(convert(build(bank)));

    THEN("Repeated and identical steps share one pattern") {
      REQUIRE(module.patternCount == 2);
      REQUIRE(module.orders ==
              std::vector<uint8_t>{0, 1, 0, 0, 1, ORDER_END_MARKER});
      REQUIRE(module.patterns[0][0][0] == Cell{S3M_C4, 0, NO_VOLUME, 0, 0});
      REQUIRE(module.patterns[1][0][0] == Cell{S3M_C5, 0, NO_VOLUME, 0, 0});
    }
  }

  GIVEN("An even number of orders") {
    MusicBank bank;
    bank.steps = {Step{ChannelData{PERIOD_C4, DELAY | 64}},
                  Step{ChannelData{PERIOD_C5, DELAY | 64}}};
    bank.orders[0] = {1, 0};
    const Module module = readModule(convert(build(bank)));

    THEN("No end marker is added") {
      REQUIRE(module.orderCount == 2);
      REQUIRE(module.orders == std::vector<uint8_t>{0, 1});
      REQUIRE(module.patterns[0][0][0].note == S3M_C5);
    }
  }
}

SCENARIO("convert turns AMOS pattern commands into S3M cells") {
  GIVEN("A lead channel that uses every command and a bass channel holding "
        "one note") {
    const ChannelData lead = {SET_SAMPLE | 0,
                              PERIOD_C4,
                              SET_VOLUME | 48,
                              DELAY | 1,
                              PERIOD_C3,
                              ARPEGGIO | 0x37,
                              DELAY | 1,
                              TONE_PORTAMENTO | 0x10,
                              DELAY | 1,
                              VIBRATO | 0x48,
                              STOP_EFFECT,
                              DELAY | 1,
                              SLIDE_UP | 0x05,
                              DELAY | 1,
                              SLIDE_DOWN | 0x06,
                              DELAY | 1,
                              VOLUME_SLIDE | 0x0F,
                              DELAY | 1,
                              SET_VOLUME | 80,
                              DELAY | 1,
                              POSITION_JUMP | 0x02,
                              DELAY | 1,
                              UNTRANSLATED_COMMAND | 0x12,
                              0x0000,
                              DELAY | 1,
                              PERIOD_NEAR_C4,
                              DELAY | 2,
                              PERIOD_FLAG_BITS | PERIOD_C5,
                              DELAY | 1,
                              SET_SAMPLE | 1,
                              PERIOD_C_SHARP_4,
                              DELAY | 1,
                              PERIOD_FLAG_BITS,
                              DELAY | 1,
                              END};
    const ChannelData bass = {SET_SAMPLE | 0, PERIOD_C2, DELAY | 64};
    MusicBank bank = songOfOnePattern(Step{lead, {}, bass, {}});
    bank.samples = {{"lead", {0, 0}}, {"bass", {0, 0}}};
    const Module module = readModule(convert(build(bank)));
    REQUIRE(module.patternCount == 1);
    const Rows &rows = module.patterns[0];

    THEN("A note plays the current sample at the volume set before its "
         "delay") {
      REQUIRE(rows[0][0] == Cell{S3M_C4, 1, 48, 0, 0});
      REQUIRE(rows[0][2] == Cell{S3M_C2, 1, NO_VOLUME, 0, 0});
    }

    THEN("Each effect command becomes the matching S3M effect") {
      REQUIRE(rows[1][0] == Cell{S3M_C3, 1, NO_VOLUME, EFFECT_ARPEGGIO, 0x37});
      REQUIRE(rows[2][0] == effect(EFFECT_TONE_PORTA, 0x10));
      REQUIRE(rows[4][0] == effect(EFFECT_PORTA_UP, 0x05));
      REQUIRE(rows[5][0] == effect(EFFECT_PORTA_DOWN, 0x06));
      REQUIRE(rows[6][0] == effect(EFFECT_VOLUME_SLIDE, 0x0F));
      REQUIRE(rows[8][0] == effect(EFFECT_POSITION_JUMP, 0x02));
    }

    THEN("A stopped effect, an untranslated command and a zero word leave "
         "their rows empty") {
      REQUIRE(rows[3][0] == Cell{});
      REQUIRE(rows[9][0] == Cell{});
      REQUIRE(rows[11][0] == Cell{});
    }

    THEN("A volume above 63 is clamped") {
      REQUIRE(rows[7][0] == Cell{NO_NOTE, 0, 63, 0, 0});
    }

    THEN("A period between notes gets the nearest note and bits above the "
         "period are ignored") {
      REQUIRE(rows[10][0] == Cell{S3M_C4, 1, NO_VOLUME, 0, 0});
      REQUIRE(rows[12][0] == Cell{S3M_C5, 1, NO_VOLUME, 0, 0});
    }

    THEN("A sample change applies to the notes after it") {
      REQUIRE(rows[13][0] == Cell{S3M_C_SHARP_4, 2, NO_VOLUME, 0, 0});
    }

    THEN("A note word without period bits gives the instrument but no note") {
      REQUIRE(rows[14][0] == Cell{NO_NOTE, 2, NO_VOLUME, 0, 0});
    }

    THEN("END cuts the note and breaks to the next pattern on its row") {
      REQUIRE(rows[15][0] ==
              Cell{NOTE_OFF, 0, NO_VOLUME, EFFECT_PATTERN_BREAK, 0});
      for (int channel = 0; channel < CHANNELS; ++channel) {
        REQUIRE(rowsEmpty(rows, 16, channel));
      }
    }

    THEN("Channels without data stay empty") {
      REQUIRE(rowsEmpty(rows, 0, 1));
      REQUIRE(rowsEmpty(rows, 0, 3));
      REQUIRE(rowsEmpty(rows, 1, 2));
    }
  }
}

SCENARIO("convert splits an AMOS tempo change into S3M speed and tempo") {
  GIVEN("Tempo changes on channel 0, one while channel 1 has an effect, then "
        "one on channel 1 while channel 0 has an effect") {
    const ChannelData first = {SET_TEMPO | 25, DELAY | 1,      SET_TEMPO | 0,
                               DELAY | 1,      SET_TEMPO | 40, DELAY | 1,
                               VIBRATO | 0x44, DELAY | 61};
    const ChannelData second = {DELAY | 2, VIBRATO | 0x11, DELAY | 1,
                                SET_TEMPO | 25, DELAY | 61};
    const Module module =
        readModule(convert(build(songOfOnePattern(Step{first, second}))));
    const Rows &rows = module.patterns.at(0);

    THEN("The speed goes on the command's channel and the tempo on the "
         "first other channel without an effect") {
      REQUIRE(rows[0][0] == effect(EFFECT_SPEED, 4));
      REQUIRE(rows[0][1] == effect(EFFECT_TEMPO, 125));
      REQUIRE(rows[2][0] == effect(EFFECT_SPEED, 3));
      REQUIRE(rows[2][1] == effect(EFFECT_VIBRATO, 0x11));
      REQUIRE(rows[2][2] == effect(EFFECT_TEMPO, 150));
      REQUIRE(rows[3][0] == effect(EFFECT_VIBRATO, 0x44));
      REQUIRE(rows[3][1] == effect(EFFECT_SPEED, 4));
      REQUIRE(rows[3][2] == effect(EFFECT_TEMPO, 125));
    }

    THEN("A tempo of 0 is ignored") {
      REQUIRE(rows[1][0] == Cell{});
      REQUIRE(rows[1][1] == Cell{});
    }
  }
}

SCENARIO("convert ends a pattern on the first END of any channel") {
  GIVEN("Channel 1 ends on row 3 where channel 0 has an effect, and channel 2 "
        "ends on row 5") {
    const ChannelData effectOnRow3 = {DELAY | 3, VIBRATO | 0x33, DELAY | 61};
    const ChannelData endOnRow3 = {DELAY | 3, END};
    const ChannelData endOnRow5 = {DELAY | 5, END};
    const Module module = readModule(convert(
        build(songOfOnePattern(Step{effectOnRow3, endOnRow3, endOnRow5}))));
    const Rows &rows = module.patterns.at(0);

    THEN("The break goes on row 3 of the first channel without an effect") {
      REQUIRE(rows[3][0] == effect(EFFECT_VIBRATO, 0x33));
      REQUIRE(rows[3][1] ==
              Cell{NOTE_OFF, 0, NO_VOLUME, EFFECT_PATTERN_BREAK, 0});
      REQUIRE(rows[3][2] == Cell{});
    }

    THEN("The later END still cuts its own channel") {
      REQUIRE(rows[5][2] == Cell{NOTE_OFF, 0, NO_VOLUME, 0, 0});
    }
  }
}

SCENARIO("convert keeps what a channel sets when its data ends without a "
         "delay") {
  GIVEN("A bank without samples whose last channel ends the bank after a "
        "sample, note, volume and slide") {
    const ChannelData unfinished = {SET_SAMPLE | 1, PERIOD_C5, SET_VOLUME | 20,
                                    SLIDE_UP | 3};
    const Module module = readModule(convert(build(songOfOnePattern(
        Step{ChannelData{}, ChannelData{}, ChannelData{}, unfinished}))));

    THEN("They all land on the channel's current row") {
      REQUIRE(module.patterns.at(0)[0][3] ==
              Cell{S3M_C5, 2, 20, EFFECT_PORTA_UP, 3});
      REQUIRE(rowsEmpty(module.patterns[0], 1, 3));
    }

    THEN("There are no instruments") { REQUIRE(module.instrumentCount == 0); }
  }

  GIVEN("A last channel that ends the bank with a volume and an effect but no "
        "note") {
    const ChannelData unfinished = {DELAY | 2, SET_VOLUME | 9, VIBRATO | 0x21};
    const Module module = readModule(convert(build(songOfOnePattern(
        Step{ChannelData{}, ChannelData{}, ChannelData{}, unfinished}))));

    THEN("Only the volume and the effect land on its current row") {
      REQUIRE(module.patterns.at(0)[2][3] ==
              Cell{NO_NOTE, 0, 9, EFFECT_VIBRATO, 0x21});
      REQUIRE(rowsEmpty(module.patterns[0], 3, 3));
    }
  }

  GIVEN("A last channel that ends the bank right after a delay") {
    const ChannelData finished = {PERIOD_C4, DELAY | 2};
    const Module module = readModule(convert(build(songOfOnePattern(
        Step{ChannelData{}, ChannelData{}, ChannelData{}, finished}))));

    THEN("Nothing is added after the delay") {
      REQUIRE(module.patterns.at(0)[0][3] == Cell{S3M_C4, 0, NO_VOLUME, 0, 0});
      REQUIRE(rowsEmpty(module.patterns[0], 1, 3));
    }
  }
}

SCENARIO("convert describes each AMOS sample as an S3M instrument") {
  GIVEN("A looped sample, a sample with a two-word loop and a sample whose "
        "loop runs past its end") {
    MusicBank bank = songOfOnePattern(Step{ChannelData{DELAY | 64}});
    bank.samples = {{"kick", {0, 1, -1, 127, -128, 64, -64, 2}, 70, 2, 3},
                    {"snare", {10, 20, 30, 40, 50, 60}, 40, 0, 2},
                    {"hat", {5, 6, 7, 8, 9, 10, 11, 12}, 63, 4, 8}};
    const Module module = readModule(convert(build(bank)));
    REQUIRE(module.instruments.size() == 3);
    const Instrument &kick = module.instruments[0];
    const Instrument &snare = module.instruments[1];
    const Instrument &hat = module.instruments[2];

    THEN("Each is an 8287 Hz sample with a numbered file name, the AMOS name "
         "and the SCRS signature") {
      for (std::size_t i = 0; i < module.instruments.size(); ++i) {
        const Instrument &instrument = module.instruments[i];
        REQUIRE(instrument.type == 1);
        REQUIRE(instrument.dosName ==
                "SAMPLE0" + std::to_string(i + 1) + ".RAW");
        REQUIRE(instrument.c2spd == AMIGA_SAMPLE_RATE);
        REQUIRE(instrument.signature == "SCRS");
      }
      REQUIRE(kick.name == "kick");
      REQUIRE(snare.name == "snare");
      REQUIRE(hat.name == "hat");
    }

    THEN("Lengths come from the word counts and volumes are clamped to 63") {
      REQUIRE(kick.length == 8);
      REQUIRE(snare.length == 6);
      REQUIRE(hat.length == 8);
      REQUIRE(kick.volume == 63);
      REQUIRE(snare.volume == 40);
      REQUIRE(hat.volume == 63);
    }

    THEN("A loop longer than two words is kept") {
      REQUIRE(kick.loopStart == 2);
      REQUIRE(kick.loopEnd == 8);
      REQUIRE(kick.flags == 1);
    }

    THEN("A two-word loop is not a loop") {
      REQUIRE(snare.loopStart == 0);
      REQUIRE(snare.loopEnd == 4);
      REQUIRE(snare.flags == 0);
    }

    THEN("A loop past the end is cut at the end, and is dropped when only "
         "four bytes remain") {
      REQUIRE(hat.loopStart == 4);
      REQUIRE(hat.loopEnd == 8);
      REQUIRE(hat.flags == 0);
    }

    THEN("The signed PCM is stored unsigned") {
      REQUIRE(kick.data == std::vector<uint8_t>{0x80, 0x81, 0x7F, 0xFF, 0x00,
                                                0xC0, 0x40, 0x82});
      REQUIRE(snare.data ==
              std::vector<uint8_t>{0x8A, 0x94, 0x9E, 0xA8, 0xB2, 0xBC});
    }
  }

  GIVEN("A last sample that claims two more words than the bank holds") {
    MusicBank bank = songOfOnePattern(Step{ChannelData{DELAY | 64}});
    bank.samples = {{"tail", {1, 2, 3, 4}, 64, 0, 0, uint16_t{4}}};
    const Module module = readModule(convert(build(bank)));

    THEN("The missing bytes are filled with silence") {
      REQUIRE(module.instruments.at(0).length == 8);
      REQUIRE(
          module.instruments[0].data ==
          std::vector<uint8_t>{0x81, 0x82, 0x83, 0x84, 0x80, 0x80, 0x80, 0x80});
    }
  }
}

SCENARIO("convert copes with damaged sample, song and track tables") {
  MusicBank bank = songOfOnePattern(Step{ChannelData{PERIOD_C4, DELAY | 64}});
  bank.samples = {{"one", {1, 2}}, {"two", {3, 4}}};
  auto abk = build(bank);

  GIVEN("A sample pointer past the end of the bank") {
    patchLong(abk, AMBK_HEADER_SIZE + SAMPLES_POINTER, 0xFFFF);

    THEN("The song converts without instruments") {
      const Module module = readModule(convert(abk));
      REQUIRE(module.instrumentCount == 0);
      REQUIRE(module.patterns.at(0)[0][0].note == S3M_C4);
    }
  }

  GIVEN("A sample table that claims no samples") {
    patchWord(abk, sectionAt(abk, SAMPLES_POINTER), 0);

    THEN("There are no instruments") {
      REQUIRE(readModule(convert(abk)).instrumentCount == 0);
    }
  }

  GIVEN("A sample table that claims more than 64 samples") {
    patchWord(abk, sectionAt(abk, SAMPLES_POINTER), 65);

    THEN("There are no instruments") {
      REQUIRE(readModule(convert(abk)).instrumentCount == 0);
    }
  }

  GIVEN("A sample table that claims a third sample past the end of the bank") {
    patchWord(abk, sectionAt(abk, SAMPLES_POINTER), 3);

    THEN("Only the complete descriptors become instruments") {
      const Module module = readModule(convert(abk));
      REQUIRE(module.instrumentCount == 2);
      REQUIRE(module.instruments[1].name == "two");
    }
  }

  GIVEN("A song pointer past the end of the bank") {
    patchLong(abk, AMBK_HEADER_SIZE + SONGS_POINTER, 0xFFFF);

    THEN("There is nothing to play") {
      REQUIRE_THROWS_WITH(convert(abk), "Empty song");
    }
  }

  GIVEN("A song table pointing past the end of the bank") {
    patchLong(abk, sectionAt(abk, SONGS_POINTER) + 2, 0xFFFF);

    THEN("There is nothing to play") {
      REQUIRE_THROWS_WITH(convert(abk), "Empty song");
    }
  }

  GIVEN("A track pointer past the end of the bank") {
    patchLong(abk, AMBK_HEADER_SIZE + TRACKS_POINTER, 0xFFFF);

    THEN("Each order plays one empty pattern") {
      const Module module = readModule(convert(abk));
      REQUIRE(module.patternCount == 1);
      for (int channel = 0; channel < CHANNELS; ++channel) {
        REQUIRE(rowsEmpty(module.patterns[0], 0, channel));
      }
    }
  }

  GIVEN("An order that names a step the track table does not have") {
    MusicBank shortTable = bank;
    shortTable.orders[0] = {0, 7};
    const Module module = readModule(convert(build(shortTable)));

    THEN("That position plays an empty pattern") {
      REQUIRE(module.patternCount == 2);
      REQUIRE(module.orders == std::vector<uint8_t>{0, 1});
      REQUIRE(rowsEmpty(module.patterns[1], 0, 0));
    }
  }
}

SCENARIO("convert refuses more distinct patterns than an S3M file holds") {
  GIVEN("A song with 254 distinct patterns") {
    const Module module =
        readModule(convert(build(songWithDistinctPatterns(254))));

    THEN("All of them are stored in order") {
      REQUIRE(module.patternCount == 254);
      REQUIRE(module.orders.size() == 254);
      REQUIRE(module.orders.front() == 0);
      REQUIRE(module.orders.back() == 253);
    }
  }

  GIVEN("A song with 255 distinct patterns") {
    const auto abk = build(songWithDistinctPatterns(255));

    THEN("It throws") {
      REQUIRE_THROWS_WITH(convert(abk),
                          "Module has more than 254 distinct patterns");
    }
  }
}
