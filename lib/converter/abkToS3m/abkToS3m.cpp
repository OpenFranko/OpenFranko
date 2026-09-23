#include "abkToS3m.h"
#include "../../helpers/helpers.h"
#include "../gameData/gameData.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

namespace openfranko::lib::converter::abkToS3m {

using helpers::padTo16;
using helpers::pushLittleEndian16;

namespace {

constexpr uint8_t AMOS_CMD_END = 0x80;
constexpr uint8_t AMOS_CMD_SET_VOLUME = 0x83;
constexpr uint8_t AMOS_CMD_STOP_EFFECT = 0x84;
constexpr uint8_t AMOS_CMD_SET_TEMPO = 0x88;
constexpr uint8_t AMOS_CMD_SET_SAMPLE = 0x89;
constexpr uint8_t AMOS_CMD_ARPEGGIO = 0x8A;
constexpr uint8_t AMOS_CMD_PORTAMENTO = 0x8B;
constexpr uint8_t AMOS_CMD_VIBRATO = 0x8C;
constexpr uint8_t AMOS_CMD_VOLUME_SLIDE = 0x8D;
constexpr uint8_t AMOS_CMD_SLIDE_UP = 0x8E;
constexpr uint8_t AMOS_CMD_SLIDE_DOWN = 0x8F;
constexpr uint8_t AMOS_CMD_DELAY = 0x90;
constexpr uint8_t AMOS_CMD_POSITION_JUMP = 0x91;

constexpr uint8_t S3M_EFFECT_SPEED = 1;
constexpr uint8_t S3M_EFFECT_POSITION_JUMP = 2;
constexpr uint8_t S3M_EFFECT_PATTERN_BREAK = 3;
constexpr uint8_t S3M_EFFECT_VOLUME_SLIDE = 4;
constexpr uint8_t S3M_EFFECT_PORTA_DOWN = 5;
constexpr uint8_t S3M_EFFECT_PORTA_UP = 6;
constexpr uint8_t S3M_EFFECT_TONE_PORTA = 7;
constexpr uint8_t S3M_EFFECT_VIBRATO = 8;
constexpr uint8_t S3M_EFFECT_ARPEGGIO = 10;
constexpr uint8_t S3M_EFFECT_TEMPO = 20;

constexpr uint8_t S3M_NOTE_NONE = 0xFF;
constexpr uint8_t S3M_NOTE_OFF = 0xFE;
constexpr uint8_t S3M_VOLUME_NONE = 0xFF;

constexpr int NUM_CHANNELS = 4;
constexpr size_t MAX_S3M_PATTERNS = 254;
constexpr uint16_t FRANKO_MENU_TEMPO = 37;

constexpr uint16_t PERIOD_TABLE[] = {
    1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960, 906,
    856,  808,  762,  720,  678,  640,  604,  570,  538,  508,  480, 453,
    428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240, 226,
    214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120, 113,
    107,  101,  95,   90,   85,   80,   75,   71,   67,   63,   60,  56,
};
constexpr int PERIOD_TABLE_SIZE =
    sizeof(PERIOD_TABLE) / sizeof(PERIOD_TABLE[0]);

uint8_t periodToS3mNote(uint16_t period) {
  if (period == 0) {
    return S3M_NOTE_NONE;
  }
  int best = 0;
  int bestDist = 0x7FFF;
  for (int i = 0; i < PERIOD_TABLE_SIZE; i++) {
    int dist = static_cast<int>(PERIOD_TABLE[i]) - static_cast<int>(period);
    if (dist < 0) {
      dist = -dist;
    }
    if (dist < bestDist) {
      bestDist = dist;
      best = i;
    }
  }
  int octave = best / 12 + 2;
  int semitone = best % 12;
  return static_cast<uint8_t>((octave << 4) | semitone);
}

struct AmosSample {
  uint32_t pcmOffset;
  uint32_t length;
  uint32_t loopStart;
  uint16_t loopLen;
  uint16_t volume;
  char name[17];
};

struct RowEvent {
  uint8_t note;
  uint8_t instrument;
  uint8_t volume;
  uint8_t effect;
  uint8_t effectParam;
};

struct Pattern {
  RowEvent channels[NUM_CHANNELS][64];
};

std::vector<AmosSample> parseSamples(const uint8_t *music, size_t musicSize,
                                     size_t sampleInfoOff) {
  std::vector<AmosSample> samples;
  if (sampleInfoOff + 2 > musicSize) {
    return samples;
  }
  const std::vector<uint8_t> musicVec(music, music + musicSize);
  helpers::BigEndianReader reader(musicVec);
  auto read32 = [&](size_t offset) { return reader.readUint32(offset); };
  auto read16 = [&](size_t offset) { return reader.readUint16(offset); };
  uint16_t count = read16(sampleInfoOff);
  if (count == 0 || count > 64) {
    return samples;
  }
  size_t descOff = sampleInfoOff + 2;
  for (uint16_t i = 0; i < count; i++) {
    size_t off = descOff + static_cast<size_t>(i) * 32;
    if (off + 32 > musicSize) {
      break;
    }
    AmosSample s{};
    s.pcmOffset = read32(off);
    uint32_t loopPos = read32(off + 4);
    s.loopLen = read16(off + 10);
    s.volume = read16(off + 12);
    s.length = static_cast<uint32_t>(read16(off + 14)) * 2;
    s.loopStart = (loopPos > s.pcmOffset) ? (loopPos - s.pcmOffset) : 0;
    std::memcpy(s.name, music + off + 16, 16);
    s.name[16] = '\0';
    samples.push_back(s);
  }
  return samples;
}

struct SongInfo {
  uint16_t speed;
  std::vector<uint16_t> orders[NUM_CHANNELS];
  char name[17];
};

SongInfo parseSong(const uint8_t *music, size_t musicSize, size_t songOff) {
  SongInfo info{};
  info.speed = 17;
  const std::vector<uint8_t> musicVec(music, music + musicSize);
  helpers::BigEndianReader reader(musicVec);
  auto read16 = [&](size_t offset) { return reader.readUint16(offset); };
  if (songOff + 6 > musicSize) {
    return info;
  }

  const uint32_t songDataOff = reader.readUint32(songOff + 2);
  size_t songBase = songOff + songDataOff;
  if (songBase + 28 > musicSize) {
    return info;
  }

  uint16_t chOff[NUM_CHANNELS];
  chOff[0] = read16(songBase);
  chOff[1] = read16(songBase + 2);
  chOff[2] = read16(songBase + 4);
  chOff[3] = read16(songBase + 6);
  info.speed = read16(songBase + 8);
  std::memcpy(info.name, music + songBase + 12, 16);
  info.name[16] = '\0';

  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    size_t pos = songBase + chOff[ch];
    while (pos + 2 <= musicSize) {
      uint16_t val = read16(pos);
      if (val & 0x8000) {
        break;
      }
      info.orders[ch].push_back(val);
      pos += 2;
    }
  }
  return info;
}

struct TrackInfo {
  uint16_t numSteps;
  std::vector<uint16_t> offsets;
  size_t trackDataBase;
};

TrackInfo parseTrackData(const uint8_t *music, size_t musicSize,
                         size_t trackOff) {
  TrackInfo info{};
  info.trackDataBase = trackOff;
  const std::vector<uint8_t> musicVec(music, music + musicSize);
  helpers::BigEndianReader reader(musicVec);
  auto read16 = [&](size_t offset) { return reader.readUint16(offset); };
  if (trackOff + 2 > musicSize) {
    return info;
  }
  info.numSteps = read16(trackOff);
  size_t numOffsets = static_cast<size_t>(info.numSteps) * NUM_CHANNELS;
  for (size_t i = 0; i < numOffsets; i++) {
    size_t pos = trackOff + 2 + i * 2;
    if (pos + 2 > musicSize) {
      break;
    }
    info.offsets.push_back(read16(pos));
  }
  return info;
}

struct DecodedPattern {
  Pattern pat;
  int endRow;
};

void decodeChannel(Pattern &pat, int ch, const uint8_t *music, size_t musicSize,
                   const TrackInfo &track, uint16_t stepIdx,
                   int &channelEndRow) {
  size_t tableIdx = static_cast<size_t>(stepIdx) * NUM_CHANNELS + ch;
  if (tableIdx >= track.offsets.size()) {
    return;
  }

  if (track.offsets[tableIdx] == 0) {
    return;
  }

  auto readUint16Safe = [&](size_t offset) -> uint16_t {
    if (offset + 2 > musicSize) {
      return 0;
    }
    return static_cast<uint16_t>((music[offset] << 8) | music[offset + 1]);
  };

  size_t pos = track.trackDataBase + track.offsets[tableIdx];
  int row = 0;
  uint8_t curSample = 0;
  uint8_t curVolume = S3M_VOLUME_NONE;
  uint8_t pendingEffect = 0;
  uint8_t pendingParam = 0;
  bool noteSet = false;
  uint16_t notePeriod = 0;

  while (pos + 2 <= musicSize && row < 64) {
    uint16_t cmd = readUint16Safe(pos);
    pos += 2;
    uint8_t hi = cmd >> 8;
    uint8_t lo = cmd & 0xFF;

    switch (hi) {
    case AMOS_CMD_DELAY: {
      auto &ev = pat.channels[ch][row];
      if (noteSet) {
        ev.note = periodToS3mNote(notePeriod);
        ev.instrument = curSample;
        noteSet = false;
      }
      if (curVolume != S3M_VOLUME_NONE) {
        ev.volume = curVolume;
        curVolume = S3M_VOLUME_NONE;
      }
      if (pendingEffect != 0) {
        ev.effect = pendingEffect;
        ev.effectParam = pendingParam;
        pendingEffect = 0;
        pendingParam = 0;
      }
      row += lo;
      break;
    }
    case AMOS_CMD_END:
      if (row < 64) {
        pat.channels[ch][row].note = S3M_NOTE_OFF;
        pat.channels[ch][row].instrument = 0;
      }
      channelEndRow = row;
      return;
    case AMOS_CMD_SET_VOLUME:
      curVolume = std::min(lo, static_cast<uint8_t>(63));
      break;
    case AMOS_CMD_SET_SAMPLE:
      curSample = lo + 1;
      break;
    case AMOS_CMD_STOP_EFFECT:
      pendingEffect = 0;
      pendingParam = 0;
      break;
    case AMOS_CMD_SET_TEMPO:
      if (lo > 0) {
        pendingEffect = S3M_EFFECT_SPEED;
        pendingParam = lo;
      }
      break;
    case AMOS_CMD_ARPEGGIO:
      pendingEffect = S3M_EFFECT_ARPEGGIO;
      pendingParam = lo;
      break;
    case AMOS_CMD_PORTAMENTO:
      pendingEffect = S3M_EFFECT_TONE_PORTA;
      pendingParam = lo;
      break;
    case AMOS_CMD_VIBRATO:
      pendingEffect = S3M_EFFECT_VIBRATO;
      pendingParam = lo;
      break;
    case AMOS_CMD_VOLUME_SLIDE:
      pendingEffect = S3M_EFFECT_VOLUME_SLIDE;
      pendingParam = lo;
      break;
    case AMOS_CMD_SLIDE_UP:
      pendingEffect = S3M_EFFECT_PORTA_UP;
      pendingParam = lo;
      break;
    case AMOS_CMD_SLIDE_DOWN:
      pendingEffect = S3M_EFFECT_PORTA_DOWN;
      pendingParam = lo;
      break;
    case AMOS_CMD_POSITION_JUMP:
      pendingEffect = S3M_EFFECT_POSITION_JUMP;
      pendingParam = lo;
      break;
    default:
      if (hi < 0x80 && cmd != 0) {
        notePeriod = cmd & 0x0FFF;
        noteSet = true;
      }
      break;
    }
  }

  if (channelEndRow == 64 && row < 64 &&
      (noteSet || pendingEffect != 0 || curVolume != S3M_VOLUME_NONE)) {
    auto &ev = pat.channels[ch][row];
    if (noteSet) {
      ev.note = periodToS3mNote(notePeriod);
      ev.instrument = curSample;
    }
    if (curVolume != S3M_VOLUME_NONE) {
      ev.volume = curVolume;
    }
    if (pendingEffect != 0) {
      ev.effect = pendingEffect;
      ev.effectParam = pendingParam;
    }
  }
}

DecodedPattern decodePattern(const uint8_t *music, size_t musicSize,
                             const TrackInfo &track,
                             const uint16_t stepIndices[NUM_CHANNELS]) {
  Pattern pat{};
  for (int row = 0; row < 64; row++) {
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      pat.channels[ch][row] = {S3M_NOTE_NONE, 0, S3M_VOLUME_NONE, 0, 0};
    }
  }

  int channelEndRows[NUM_CHANNELS] = {64, 64, 64, 64};
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    decodeChannel(pat, ch, music, musicSize, track, stepIndices[ch],
                  channelEndRows[ch]);
  }

  int endRow = 64;
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    if (channelEndRows[ch] < endRow) {
      endRow = channelEndRows[ch];
    }
  }
  if (endRow < 64) {
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      if (pat.channels[ch][endRow].effect == 0) {
        pat.channels[ch][endRow].effect = S3M_EFFECT_PATTERN_BREAK;
        pat.channels[ch][endRow].effectParam = 0;
        break;
      }
    }
  }

  return {pat, endRow};
}

std::vector<uint8_t> packPattern(const Pattern &pat) {
  std::vector<uint8_t> packed;
  packed.push_back(0);
  packed.push_back(0);

  for (int row = 0; row < 64; row++) {
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      const auto &ev = pat.channels[ch][row];
      uint8_t what = 0;
      if (ev.note != S3M_NOTE_NONE || ev.instrument != 0) {
        what |= 0x20;
      }
      if (ev.volume != S3M_VOLUME_NONE) {
        what |= 0x40;
      }
      if (ev.effect != 0) {
        what |= 0x80;
      }
      if (what == 0) {
        continue;
      }
      what |= static_cast<uint8_t>(ch);
      packed.push_back(what);
      if (what & 0x20) {
        packed.push_back(ev.note);
        packed.push_back(ev.instrument);
      }
      if (what & 0x40) {
        packed.push_back(ev.volume);
      }
      if (what & 0x80) {
        packed.push_back(ev.effect);
        packed.push_back(ev.effectParam);
      }
    }
    packed.push_back(0);
  }

  uint16_t len = static_cast<uint16_t>(packed.size());
  packed[0] = static_cast<uint8_t>(len);
  packed[1] = static_cast<uint8_t>(len >> 8);
  return packed;
}

struct SpeedTempo {
  uint8_t speed;
  uint8_t tempo;
  bool hasTempo;
};

SpeedTempo amosTempoToS3m(uint8_t amosTempo) {
  if (amosTempo == 0) {
    return {6, 134, false};
  }
  int speed = (100 + amosTempo / 2) / amosTempo;
  if (speed < 1)
    speed = 1;
  if (speed > 31)
    speed = 31;
  int bpm = (5 * speed * amosTempo + 2) / 4;
  if (bpm < 32)
    bpm = 32;
  if (bpm > 255)
    bpm = 255;
  return {static_cast<uint8_t>(speed), static_cast<uint8_t>(bpm), true};
}

void fixSpeedEffects(Pattern &pat) {
  for (int row = 0; row < 64; row++) {
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      auto &ev = pat.channels[ch][row];
      if (ev.effect != S3M_EFFECT_SPEED) {
        continue;
      }
      auto st = amosTempoToS3m(ev.effectParam);
      ev.effectParam = st.speed;
      if (st.hasTempo) {
        for (int ch2 = 0; ch2 < NUM_CHANNELS; ch2++) {
          if (ch2 != ch && pat.channels[ch2][row].effect == 0) {
            pat.channels[ch2][row].effect = S3M_EFFECT_TEMPO;
            pat.channels[ch2][row].effectParam = st.tempo;
            break;
          }
        }
      }
    }
  }
}

std::vector<Pattern> decodeAllPatterns(const uint8_t *music, size_t musicSize,
                                       const TrackInfo &track,
                                       const SongInfo &song,
                                       std::vector<uint8_t> &orderList) {
  std::vector<Pattern> patterns;

  size_t songLen = 0;
  for (int ch = 0; ch < NUM_CHANNELS; ch++) {
    songLen = std::max(songLen, song.orders[ch].size());
  }

  for (size_t pos = 0; pos < songLen; pos++) {
    uint16_t steps[NUM_CHANNELS];
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      steps[ch] = pos < song.orders[ch].size() ? song.orders[ch][pos] : 0;
    }

    auto decoded = decodePattern(music, musicSize, track, steps);
    fixSpeedEffects(decoded.pat);
    bool found = false;
    for (size_t pi = 0; pi < patterns.size(); pi++) {
      if (std::memcmp(&patterns[pi], &decoded.pat, sizeof(Pattern)) == 0) {
        orderList.push_back(static_cast<uint8_t>(pi));
        found = true;
        break;
      }
    }
    if (!found) {
      if (patterns.size() >= MAX_S3M_PATTERNS) {
        throw std::runtime_error("Module has more than " +
                                 std::to_string(MAX_S3M_PATTERNS) +
                                 " distinct patterns");
      }
      orderList.push_back(static_cast<uint8_t>(patterns.size()));
      patterns.push_back(decoded.pat);
    }
  }

  return patterns;
}

void writeS3mHeader(std::vector<uint8_t> &s3m, const SongInfo &song,
                    uint16_t ordNum, uint16_t insNum, uint16_t patNum,
                    uint8_t speed, uint8_t tempo) {
  s3m.resize(96, 0);

  size_t nameLen = std::strlen(song.name);
  if (nameLen > 28) {
    nameLen = 28;
  }
  std::memcpy(s3m.data(), song.name, nameLen);
  s3m[0x1C] = 0x1A;
  s3m[0x1D] = 0x10;
  s3m[0x20] = static_cast<uint8_t>(ordNum);
  s3m[0x21] = static_cast<uint8_t>(ordNum >> 8);
  s3m[0x22] = static_cast<uint8_t>(insNum);
  s3m[0x23] = static_cast<uint8_t>(insNum >> 8);
  s3m[0x24] = static_cast<uint8_t>(patNum);
  s3m[0x25] = static_cast<uint8_t>(patNum >> 8);
  s3m[0x28] = 0x20;
  s3m[0x29] = 0x13;
  s3m[0x2A] = 0x02;
  s3m[0x2C] = 'S';
  s3m[0x2D] = 'C';
  s3m[0x2E] = 'R';
  s3m[0x2F] = 'M';
  s3m[0x30] = 64;
  s3m[0x31] = speed;
  s3m[0x32] = tempo;
  s3m[0x33] = 0x80 | 48;
  s3m[0x34] = 16;
  s3m[0x35] = 0xFC;

  s3m[0x40] = 0x00;
  s3m[0x41] = 0x08;
  s3m[0x42] = 0x09;
  s3m[0x43] = 0x01;
  for (int i = 4; i < 32; i++) {
    s3m[0x40 + i] = 0xFF;
  }
}

void writeInstrument(std::vector<uint8_t> &s3m, size_t insStart,
                     const AmosSample &sample, int index) {
  s3m.resize(insStart + 80, 0);
  s3m[insStart] = 1;
  char dosName[13] = {};
  snprintf(dosName, sizeof(dosName), "SAMPLE%02d.RAW", index + 1);
  std::memcpy(s3m.data() + insStart + 1, dosName, 12);

  uint32_t sampleLen = sample.length;
  uint32_t loopStart = sample.loopStart;
  uint32_t loopEnd = loopStart + static_cast<uint32_t>(sample.loopLen) * 2;
  if (loopEnd > sampleLen) {
    loopEnd = sampleLen;
  }
  bool hasLoop = sample.loopLen > 2 && loopEnd > loopStart + 4;

  s3m[insStart + 0x10] = static_cast<uint8_t>(sampleLen);
  s3m[insStart + 0x11] = static_cast<uint8_t>(sampleLen >> 8);
  s3m[insStart + 0x12] = static_cast<uint8_t>(sampleLen >> 16);
  s3m[insStart + 0x13] = static_cast<uint8_t>(sampleLen >> 24);

  s3m[insStart + 0x14] = static_cast<uint8_t>(loopStart);
  s3m[insStart + 0x15] = static_cast<uint8_t>(loopStart >> 8);
  s3m[insStart + 0x16] = static_cast<uint8_t>(loopStart >> 16);
  s3m[insStart + 0x17] = static_cast<uint8_t>(loopStart >> 24);

  s3m[insStart + 0x18] = static_cast<uint8_t>(loopEnd);
  s3m[insStart + 0x19] = static_cast<uint8_t>(loopEnd >> 8);
  s3m[insStart + 0x1A] = static_cast<uint8_t>(loopEnd >> 16);
  s3m[insStart + 0x1B] = static_cast<uint8_t>(loopEnd >> 24);

  uint8_t vol = static_cast<uint8_t>(std::min<uint16_t>(sample.volume, 63));
  s3m[insStart + 0x1C] = vol;

  if (hasLoop) {
    s3m[insStart + 0x1F] = 1;
  }

  uint32_t c2spd = gameData::audio::DEFAULT_SAMPLE_RATE;
  s3m[insStart + 0x20] = static_cast<uint8_t>(c2spd);
  s3m[insStart + 0x21] = static_cast<uint8_t>(c2spd >> 8);
  s3m[insStart + 0x22] = static_cast<uint8_t>(c2spd >> 16);
  s3m[insStart + 0x23] = static_cast<uint8_t>(c2spd >> 24);

  std::memcpy(s3m.data() + insStart + 0x30, sample.name, 16);

  s3m[insStart + 0x4C] = 'S';
  s3m[insStart + 0x4D] = 'C';
  s3m[insStart + 0x4E] = 'R';
  s3m[insStart + 0x4F] = 'S';
}

} // namespace

std::vector<uint8_t> convert(const std::vector<uint8_t> &abkData,
                             uint16_t initialAmosTempo) {
  if (abkData.size() < 24 || abkData[0] != 'A' || abkData[1] != 'm' ||
      abkData[2] != 'B' || abkData[3] != 'k') {
    throw std::runtime_error("not a valid AmBk file");
  }

  const uint8_t *music = abkData.data() + 20;
  size_t musicSize = abkData.size() - 20;

  if (musicSize < 12) {
    throw std::runtime_error("music data too small");
  }

  helpers::BigEndianReader reader(abkData);
  uint32_t sampleInfoOff = reader.readUint32(20);
  uint32_t songOff = reader.readUint32(24);
  uint32_t trackOff = reader.readUint32(28);

  auto samples = parseSamples(music, musicSize, sampleInfoOff);
  auto song = parseSong(music, musicSize, songOff);
  auto track = parseTrackData(music, musicSize, trackOff);

  std::vector<uint8_t> orderList;
  auto patterns = decodeAllPatterns(music, musicSize, track, song, orderList);
  if (patterns.empty()) {
    throw std::runtime_error("empty song");
  }

  uint16_t ordNum = static_cast<uint16_t>(orderList.size());
  if (ordNum % 2 != 0) {
    orderList.push_back(0xFF);
    ordNum++;
  }
  uint16_t insNum = static_cast<uint16_t>(samples.size());
  uint16_t patNum = static_cast<uint16_t>(patterns.size());

  uint16_t amosTempo = initialAmosTempo;
  if (amosTempo == 0) {
    amosTempo =
        std::strncmp(song.name, "e1", 2) == 0 ? FRANKO_MENU_TEMPO : song.speed;
  }

  const auto initialST =
      amosTempoToS3m(static_cast<uint8_t>(std::min<uint16_t>(amosTempo, 255)));
  const uint8_t speed = initialST.speed;
  const uint8_t tempo = initialST.tempo;

  std::vector<uint8_t> s3m;
  writeS3mHeader(s3m, song, ordNum, insNum, patNum, speed, tempo);
  s3m.insert(s3m.end(), orderList.begin(), orderList.end());

  size_t insPtrOff = s3m.size();
  for (uint16_t i = 0; i < insNum; i++) {
    pushLittleEndian16(s3m, 0);
  }

  size_t patPtrOff = s3m.size();
  for (uint16_t i = 0; i < patNum; i++) {
    pushLittleEndian16(s3m, 0);
  }

  uint8_t panning[32] = {};
  panning[0] = 0x20 | 3;
  panning[1] = 0x20 | 12;
  panning[2] = 0x20 | 12;
  panning[3] = 0x20 | 3;
  s3m.insert(s3m.end(), panning, panning + 32);

  std::vector<size_t> insOffsets(insNum);
  for (uint16_t i = 0; i < insNum; i++) {
    padTo16(s3m);
    insOffsets[i] = s3m.size();
    uint16_t paraPtr = static_cast<uint16_t>(insOffsets[i] / 16);
    s3m[insPtrOff + i * 2] = static_cast<uint8_t>(paraPtr);
    s3m[insPtrOff + i * 2 + 1] = static_cast<uint8_t>(paraPtr >> 8);
    writeInstrument(s3m, s3m.size(), samples[i], i);
  }

  for (uint16_t i = 0; i < patNum; i++) {
    padTo16(s3m);
    size_t patOff = s3m.size();
    uint16_t paraPtr = static_cast<uint16_t>(patOff / 16);
    s3m[patPtrOff + i * 2] = static_cast<uint8_t>(paraPtr);
    s3m[patPtrOff + i * 2 + 1] = static_cast<uint8_t>(paraPtr >> 8);

    auto packed = packPattern(patterns[i]);
    s3m.insert(s3m.end(), packed.begin(), packed.end());
  }

  for (uint16_t i = 0; i < insNum; i++) {
    padTo16(s3m);
    size_t sampleFileOff = s3m.size();
    uint32_t paraPtr20 = static_cast<uint32_t>(sampleFileOff / 16);
    s3m[insOffsets[i] + 0x0D] = static_cast<uint8_t>(paraPtr20 >> 16);
    s3m[insOffsets[i] + 0x0E] = static_cast<uint8_t>(paraPtr20);
    s3m[insOffsets[i] + 0x0F] = static_cast<uint8_t>(paraPtr20 >> 8);

    const size_t pcmOff = samples[i].pcmOffset;
    const size_t len = samples[i].length;
    for (size_t j = 0; j < len; j++) {
      if (static_cast<size_t>(sampleInfoOff) + pcmOff + j < musicSize) {
        uint8_t signedSample = music[sampleInfoOff + pcmOff + j];
        s3m.push_back(signedSample ^ 0x80);
      } else {
        s3m.push_back(0x80);
      }
    }
  }

  return s3m;
}

} // namespace openfranko::lib::converter::abkToS3m
