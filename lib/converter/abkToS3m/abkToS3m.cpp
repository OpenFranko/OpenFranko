#include "abkToS3m.h"
#include "../../decompressor/helpers/helpers.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace openfranko::lib::converter::abkToS3m {

namespace helpers = decompressor::helpers;

namespace {

constexpr uint16_t kPeriodTable[] = {
    1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960, 906,
    856,  808,  762,  720,  678,  640,  604,  570,  538,  508,  480, 453,
    428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240, 226,
    214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120, 113,
    107,  101,  95,   90,   85,   80,   75,   71,   67,   63,   60,  56,
};
constexpr int kPeriodTableSize = sizeof(kPeriodTable) / sizeof(kPeriodTable[0]);

uint8_t periodToS3mNote(uint16_t period) {
  if (period == 0) {
    return 0xFF;
  }
  int best = 0;
  int bestDist = 0x7FFF;
  for (int i = 0; i < kPeriodTableSize; i++) {
    int dist = static_cast<int>(kPeriodTable[i]) - static_cast<int>(period);
    if (dist < 0) {
      dist = -dist;
    }
    if (dist < bestDist) {
      bestDist = dist;
      best = i;
    }
  }
  int octave = best / 12 + 2;
  int semi = best % 12;
  return static_cast<uint8_t>((octave << 4) | semi);
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
  RowEvent channels[4][64];
};

std::vector<AmosSample> parseSamples(const uint8_t *music, size_t musicSize,
                                     size_t sampleInfoOff) {
  std::vector<AmosSample> samples;
  if (sampleInfoOff + 2 > musicSize) {
    return samples;
  }
  const std::vector<uint8_t> musicVec(music, music + musicSize);
  auto r = [&](size_t o) { return helpers::readUint32BigEndian(musicVec, o); };
  auto r16 = [&](size_t o) {
    return helpers::readUint16BigEndian(musicVec, o);
  };
  uint16_t count = r16(sampleInfoOff);
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
    s.pcmOffset = r(off);
    uint32_t loopPos = r(off + 4);
    s.loopLen = r16(off + 10);
    s.volume = r16(off + 12);
    s.length = static_cast<uint32_t>(r16(off + 14)) * 2;
    s.loopStart = (loopPos > s.pcmOffset) ? (loopPos - s.pcmOffset) : 0;
    std::memcpy(s.name, music + off + 16, 16);
    s.name[16] = '\0';
    samples.push_back(s);
  }
  return samples;
}

struct SongInfo {
  uint16_t speed;
  std::vector<uint16_t> orders[4];
  char name[17];
};

SongInfo parseSong(const uint8_t *music, size_t musicSize, size_t songOff) {
  SongInfo info{};
  info.speed = 17;
  const std::vector<uint8_t> musicVec(music, music + musicSize);
  auto r16 = [&](size_t o) {
    return helpers::readUint16BigEndian(musicVec, o);
  };
  if (songOff + 6 > musicSize) {
    return info;
  }

  uint16_t songDataOff = r16(songOff + 4);
  size_t songBase = songOff + songDataOff;
  if (songBase + 28 > musicSize) {
    return info;
  }

  uint16_t chOff[4];
  chOff[0] = r16(songBase);
  chOff[1] = r16(songBase + 2);
  chOff[2] = r16(songBase + 4);
  chOff[3] = r16(songBase + 6);
  info.speed = r16(songBase + 8);
  std::memcpy(info.name, music + songBase + 12, 16);
  info.name[16] = '\0';

  for (int ch = 0; ch < 4; ch++) {
    size_t pos = songBase + chOff[ch];
    while (pos + 2 <= musicSize) {
      uint16_t val = r16(pos);
      if (val >= 0xFF00) {
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
  auto r16 = [&](size_t o) {
    return helpers::readUint16BigEndian(musicVec, o);
  };
  if (trackOff + 2 > musicSize) {
    return info;
  }
  info.numSteps = r16(trackOff);
  size_t numOffsets = static_cast<size_t>(info.numSteps) * 4;
  for (size_t i = 0; i < numOffsets; i++) {
    size_t pos = trackOff + 2 + i * 2;
    if (pos + 2 > musicSize) {
      break;
    }
    info.offsets.push_back(r16(pos));
  }
  return info;
}

struct DecodedPattern {
  Pattern pat;
  int endRow;
};

DecodedPattern decodePattern(const uint8_t *music, size_t musicSize,
                             const TrackInfo &track,
                             const uint16_t stepIndices[4]) {
  Pattern pat{};
  for (int r = 0; r < 64; r++) {
    for (int c = 0; c < 4; c++) {
      pat.channels[c][r] = {0xFF, 0, 0xFF, 0, 0};
    }
  }

  int endRow = 64;

  auto r16at = [&](size_t o) -> uint16_t {
    if (o + 2 > musicSize) {
      return 0;
    }
    return static_cast<uint16_t>((music[o] << 8) | music[o + 1]);
  };

  int channelEndRows[4] = {64, 64, 64, 64};

  for (int ch = 0; ch < 4; ch++) {
    uint16_t stepIdx = stepIndices[ch];
    size_t tableIdx = static_cast<size_t>(stepIdx) * 4 + ch;
    if (tableIdx >= track.offsets.size()) {
      continue;
    }
    size_t dataStart = track.trackDataBase + track.offsets[tableIdx];

    int row = 0;
    size_t pos = dataStart;
    uint8_t curSample = 0;
    uint8_t curVolume = 0xFF;
    uint8_t pendingEffect = 0;
    uint8_t pendingParam = 0;
    bool noteSet = false;
    uint16_t notePeriod = 0;

    while (pos + 2 <= musicSize && row < 64) {
      uint16_t cmd = r16at(pos);
      pos += 2;
      uint8_t hi = cmd >> 8;
      uint8_t lo = cmd & 0xFF;

      if (hi == 0x90) {
        auto &ev = pat.channels[ch][row];
        if (noteSet) {
          ev.note = periodToS3mNote(notePeriod);
          ev.instrument = curSample;
          noteSet = false;
        }
        if (curVolume != 0xFF) {
          ev.volume = curVolume;
          curVolume = 0xFF;
        }
        if (pendingEffect != 0) {
          ev.effect = pendingEffect;
          ev.effectParam = pendingParam;
          pendingEffect = 0;
          pendingParam = 0;
        }
        row += lo;
      } else if (hi == 0x80) {
        if (row < 64) {
          auto &ev = pat.channels[ch][row];
          ev.note = 0xFE;
          ev.instrument = 0;
        }
        channelEndRows[ch] = row;
        break;
      } else if (hi == 0x83) {
        curVolume = std::min(lo, static_cast<uint8_t>(63));
      } else if (hi == 0x89) {
        curSample = lo + 1;
      } else if (hi == 0x84) {
        pendingEffect = 0;
        pendingParam = 0;
      } else if (hi == 0x88) {
        if (lo > 0) {
          pendingEffect = 1;
          pendingParam = lo;
        }
      } else if (hi == 0x8A) {
        pendingEffect = 10;
        pendingParam = lo;
      } else if (hi == 0x8B) {
        pendingEffect = 7;
        pendingParam = lo;
      } else if (hi == 0x8C) {
        pendingEffect = 8;
        pendingParam = lo;
      } else if (hi == 0x8D) {
        pendingEffect = 4;
        pendingParam = lo;
      } else if (hi == 0x8E) {
        pendingEffect = 6;
        pendingParam = lo;
      } else if (hi == 0x8F) {
        pendingEffect = 5;
        pendingParam = lo;
      } else if (hi == 0x91) {
        pendingEffect = 2;
        pendingParam = lo;
      } else if (hi < 0x80 && cmd != 0) {
        notePeriod = cmd & 0x0FFF;
        noteSet = true;
      }
    }

    if (channelEndRows[ch] == 64 && row < 64 &&
        (noteSet || pendingEffect != 0 || curVolume != 0xFF)) {
      auto &ev = pat.channels[ch][row];
      if (noteSet) {
        ev.note = periodToS3mNote(notePeriod);
        ev.instrument = curSample;
      }
      if (curVolume != 0xFF) {
        ev.volume = curVolume;
      }
      if (pendingEffect != 0) {
        ev.effect = pendingEffect;
        ev.effectParam = pendingParam;
      }
    }
  }

  endRow = 64;
  for (int ch = 0; ch < 4; ch++) {
    if (channelEndRows[ch] < endRow) {
      endRow = channelEndRows[ch];
    }
  }
  if (endRow < 64) {
    for (int ch2 = 0; ch2 < 4; ch2++) {
      if (pat.channels[ch2][endRow].effect == 0) {
        pat.channels[ch2][endRow].effect = 3;
        pat.channels[ch2][endRow].effectParam = 0;
        break;
      }
    }
  }

  return {pat, endRow};
}

void pushLE16(std::vector<uint8_t> &buf, uint16_t v) {
  buf.push_back(static_cast<uint8_t>(v));
  buf.push_back(static_cast<uint8_t>(v >> 8));
}

void pushLE32(std::vector<uint8_t> &buf, uint32_t v) {
  buf.push_back(static_cast<uint8_t>(v));
  buf.push_back(static_cast<uint8_t>(v >> 8));
  buf.push_back(static_cast<uint8_t>(v >> 16));
  buf.push_back(static_cast<uint8_t>(v >> 24));
}

void padTo16(std::vector<uint8_t> &buf) {
  while (buf.size() % 16 != 0) {
    buf.push_back(0);
  }
}

std::vector<uint8_t> packPattern(const Pattern &pat) {
  std::vector<uint8_t> packed;
  packed.push_back(0);
  packed.push_back(0);

  for (int row = 0; row < 64; row++) {
    for (int ch = 0; ch < 4; ch++) {
      const auto &ev = pat.channels[ch][row];
      uint8_t what = 0;
      if (ev.note != 0xFF || ev.instrument != 0) {
        what |= 0x20;
      }
      if (ev.volume != 0xFF) {
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
};

SpeedTempo amosTempoToS3m(uint8_t amosTempo) {
  if (amosTempo == 0) {
    return {6, 134};
  }
  int speed = (100 + amosTempo / 2) / amosTempo;
  if (speed < 1)
    speed = 1;
  if (speed > 31)
    speed = 31;
  int bpm = (125 * speed * amosTempo + 47) / 95;
  if (bpm < 32)
    bpm = 32;
  if (bpm > 255)
    bpm = 255;
  return {static_cast<uint8_t>(speed), static_cast<uint8_t>(bpm)};
}

void fixSpeedEffects(Pattern &pat) {
  for (int row = 0; row < 64; row++) {
    for (int ch = 0; ch < 4; ch++) {
      auto &ev = pat.channels[ch][row];
      if (ev.effect != 1) {
        continue;
      }
      auto st = amosTempoToS3m(ev.effectParam);
      ev.effectParam = st.speed;
      if (st.tempo != 134) {
        for (int ch2 = 0; ch2 < 4; ch2++) {
          if (ch2 != ch && pat.channels[ch2][row].effect == 0) {
            pat.channels[ch2][row].effect = 20;
            pat.channels[ch2][row].effectParam = st.tempo;
            break;
          }
        }
      }
    }
  }
}

} // namespace

std::vector<uint8_t> convert(const std::vector<uint8_t> &abkData) {
  if (abkData.size() < 24 || abkData[0] != 'A' || abkData[1] != 'm' ||
      abkData[2] != 'B' || abkData[3] != 'k') {
    throw std::runtime_error("not a valid AmBk file");
  }

  const uint8_t *music = abkData.data() + 20;
  size_t musicSize = abkData.size() - 20;

  if (musicSize < 12) {
    throw std::runtime_error("music data too small");
  }

  uint32_t sampleInfoOff = helpers::readUint32BigEndian(abkData, 20);
  uint32_t songOff = helpers::readUint32BigEndian(abkData, 24);
  uint32_t trackOff = helpers::readUint32BigEndian(abkData, 28);

  auto samples = parseSamples(music, musicSize, sampleInfoOff);
  auto song = parseSong(music, musicSize, songOff);
  auto track = parseTrackData(music, musicSize, trackOff);

  bool isE1 = (std::strncmp(song.name, "e1", 2) == 0);

  size_t songLen = 0;
  for (int ch = 0; ch < 4; ch++) {
    songLen = std::max(songLen, song.orders[ch].size());
  }
  if (songLen == 0) {
    throw std::runtime_error("empty song");
  }

  std::vector<Pattern> patterns;
  std::vector<uint8_t> orderList;

  for (size_t pos = 0; pos < songLen; pos++) {
    uint16_t steps[4];
    for (int ch = 0; ch < 4; ch++) {
      steps[ch] = pos < song.orders[ch].size() ? song.orders[ch][pos] : 0;
    }

    auto decoded = decodePattern(music, musicSize, track, steps);
    if (isE1) {
      for (int row = 0; row < 64; row++)
        for (int ch = 0; ch < 4; ch++)
          if (decoded.pat.channels[ch][row].effect == 1)
            decoded.pat.channels[ch][row].effect = 0,
            decoded.pat.channels[ch][row].effectParam = 0;
    } else {
      fixSpeedEffects(decoded.pat);
    }
    bool found = false;
    for (size_t pi = 0; pi < patterns.size(); pi++) {
      if (std::memcmp(&patterns[pi], &decoded.pat, sizeof(Pattern)) == 0) {
        orderList.push_back(static_cast<uint8_t>(pi));
        found = true;
        break;
      }
    }
    if (!found) {
      orderList.push_back(static_cast<uint8_t>(patterns.size()));
      patterns.push_back(decoded.pat);
    }
  }

  uint16_t ordNum = static_cast<uint16_t>(orderList.size());
  if (ordNum % 2 != 0) {
    orderList.push_back(0xFF);
    ordNum++;
  }
  uint16_t insNum = static_cast<uint16_t>(samples.size());
  uint16_t patNum = static_cast<uint16_t>(patterns.size());

  uint8_t speedDivider;
  uint8_t tempo;
  if (isE1) {
    speedDivider = 3;
    tempo = 130;
  } else {
    auto initialST = amosTempoToS3m(
        static_cast<uint8_t>(std::min<uint16_t>(song.speed, 255)));
    speedDivider = initialST.speed;
    tempo = initialST.tempo;
  }

  std::vector<uint8_t> s3m;
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
  s3m[0x31] = speedDivider;
  s3m[0x32] = tempo;
  s3m[0x33] = 0x80 | 48;
  s3m[0x34] = 16;
  s3m[0x35] = 0xFC;

  s3m[0x40] = 0x00;
  s3m[0x41] = 0x01;
  s3m[0x42] = 0x08;
  s3m[0x43] = 0x09;
  for (int i = 4; i < 32; i++) {
    s3m[0x40 + i] = 0xFF;
  }

  s3m.insert(s3m.end(), orderList.begin(), orderList.end());

  size_t insPtrOff = s3m.size();
  for (uint16_t i = 0; i < insNum; i++) {
    pushLE16(s3m, 0);
  }

  size_t patPtrOff = s3m.size();
  for (uint16_t i = 0; i < patNum; i++) {
    pushLE16(s3m, 0);
  }

  uint8_t panning[32] = {};
  panning[0] = 0x20 | 3;
  panning[1] = 0x20 | 3;
  panning[2] = 0x20 | 12;
  panning[3] = 0x20 | 12;
  s3m.insert(s3m.end(), panning, panning + 32);

  std::vector<size_t> insOffsets(insNum);
  for (uint16_t i = 0; i < insNum; i++) {
    padTo16(s3m);
    insOffsets[i] = s3m.size();
    uint16_t paraPtr = static_cast<uint16_t>(insOffsets[i] / 16);
    s3m[insPtrOff + i * 2] = static_cast<uint8_t>(paraPtr);
    s3m[insPtrOff + i * 2 + 1] = static_cast<uint8_t>(paraPtr >> 8);

    size_t insStart = s3m.size();
    s3m.resize(insStart + 80, 0);
    s3m[insStart] = 1;
    char dosName[13] = {};
    snprintf(dosName, sizeof(dosName), "SAMPLE%02d.RAW", i + 1);
    std::memcpy(s3m.data() + insStart + 1, dosName, 12);

    uint32_t sampleLen = samples[i].length;
    uint32_t loopStart = samples[i].loopStart;
    uint32_t loopEnd =
        loopStart + static_cast<uint32_t>(samples[i].loopLen) * 2;
    if (loopEnd > sampleLen) {
      loopEnd = sampleLen;
    }
    bool hasLoop = samples[i].loopLen > 2 && loopEnd > loopStart + 4;

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

    uint8_t vol =
        static_cast<uint8_t>(std::min<uint16_t>(samples[i].volume, 63));
    s3m[insStart + 0x1C] = vol;

    uint8_t flags = 0;
    if (hasLoop) {
      flags |= 1;
    }
    s3m[insStart + 0x1F] = flags;

    uint32_t c2spd = 8287;
    s3m[insStart + 0x20] = static_cast<uint8_t>(c2spd);
    s3m[insStart + 0x21] = static_cast<uint8_t>(c2spd >> 8);
    s3m[insStart + 0x22] = static_cast<uint8_t>(c2spd >> 16);
    s3m[insStart + 0x23] = static_cast<uint8_t>(c2spd >> 24);

    std::memcpy(s3m.data() + insStart + 0x30, samples[i].name, 16);

    s3m[insStart + 0x4C] = 'S';
    s3m[insStart + 0x4D] = 'C';
    s3m[insStart + 0x4E] = 'R';
    s3m[insStart + 0x4F] = 'S';
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

    uint32_t pcmOff = samples[i].pcmOffset;
    uint32_t len = samples[i].length;
    for (uint32_t j = 0; j < len; j++) {
      if (sampleInfoOff + pcmOff + j < musicSize) {
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
