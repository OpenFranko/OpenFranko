#include "abkToS3m.h"
#include "../../binary/binary.h"
#include "../gameData/gameData.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

namespace openfranko::lib::converter::abkToS3m {

namespace {

constexpr uint8_t AMOS_COMMAND_FLAG = 0x80;
constexpr uint8_t AMOS_COMMAND_END = 0x80;
constexpr uint8_t AMOS_COMMAND_SET_VOLUME = 0x83;
constexpr uint8_t AMOS_COMMAND_STOP_EFFECT = 0x84;
constexpr uint8_t AMOS_COMMAND_SET_TEMPO = 0x88;
constexpr uint8_t AMOS_COMMAND_SET_SAMPLE = 0x89;
constexpr uint8_t AMOS_COMMAND_ARPEGGIO = 0x8A;
constexpr uint8_t AMOS_COMMAND_PORTAMENTO = 0x8B;
constexpr uint8_t AMOS_COMMAND_VIBRATO = 0x8C;
constexpr uint8_t AMOS_COMMAND_VOLUME_SLIDE = 0x8D;
constexpr uint8_t AMOS_COMMAND_SLIDE_UP = 0x8E;
constexpr uint8_t AMOS_COMMAND_SLIDE_DOWN = 0x8F;
constexpr uint8_t AMOS_COMMAND_DELAY = 0x90;
constexpr uint8_t AMOS_COMMAND_POSITION_JUMP = 0x91;
constexpr uint16_t AMOS_PERIOD_MASK = 0x0FFF;
constexpr uint16_t AMOS_ORDER_END_FLAG = 0x8000;
constexpr uint16_t MAX_AMOS_TEMPO = 255;

constexpr size_t ABK_HEADER_SIZE = 20;
constexpr size_t MUSIC_HEADER_SIZE = 12;
constexpr size_t MUSIC_SAMPLES_POINTER_OFFSET = 0;
constexpr size_t MUSIC_SONGS_POINTER_OFFSET = 4;
constexpr size_t MUSIC_TRACKS_POINTER_OFFSET = 8;
constexpr size_t AMOS_NAME_SIZE = 16;

constexpr size_t SAMPLES_TABLE_OFFSET = 2;
constexpr size_t SAMPLE_DESCRIPTOR_SIZE = 32;
constexpr size_t SAMPLE_LOOP_POINTER_OFFSET = 4;
constexpr size_t SAMPLE_LOOP_LENGTH_OFFSET = 10;
constexpr size_t SAMPLE_VOLUME_OFFSET = 12;
constexpr size_t SAMPLE_LENGTH_OFFSET = 14;
constexpr size_t SAMPLE_NAME_OFFSET = 16;
constexpr uint16_t MAX_SAMPLE_COUNT = 64;

constexpr size_t SONGS_TABLE_OFFSET = 2;
constexpr size_t SONG_POINTER_SIZE = 4;
constexpr size_t SONG_HEADER_SIZE = 28;
constexpr size_t SONG_SPEED_OFFSET = 8;
constexpr size_t SONG_NAME_OFFSET = 12;
constexpr uint16_t DEFAULT_SONG_SPEED = 17;

constexpr size_t TRACKS_TABLE_OFFSET = 2;

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
constexpr uint8_t S3M_MAX_VOLUME = 63;
constexpr uint8_t S3M_PACKED_NOTE = 0x20;
constexpr uint8_t S3M_PACKED_VOLUME = 0x40;
constexpr uint8_t S3M_PACKED_EFFECT = 0x80;
constexpr uint8_t S3M_ORDER_END = 0xFF;
constexpr size_t S3M_PARAGRAPH_SIZE = 16;

constexpr uint8_t DEFAULT_S3M_SPEED = 6;
constexpr uint8_t DEFAULT_S3M_TEMPO = 134;
constexpr int MIN_S3M_SPEED = 1;
constexpr int MAX_S3M_SPEED = 31;
constexpr int MIN_S3M_TEMPO = 32;
constexpr int MAX_S3M_TEMPO = 255;

constexpr size_t S3M_HEADER_SIZE = 0x60;
constexpr size_t S3M_SONG_NAME_SIZE = 28;
constexpr size_t S3M_EOF_OFFSET = 0x1C;
constexpr uint8_t S3M_EOF_MARKER = 0x1A;
constexpr size_t S3M_TYPE_OFFSET = 0x1D;
constexpr uint8_t S3M_MODULE_TYPE = 0x10;
constexpr size_t S3M_ORDER_COUNT_OFFSET = 0x20;
constexpr size_t S3M_INSTRUMENT_COUNT_OFFSET = 0x22;
constexpr size_t S3M_PATTERN_COUNT_OFFSET = 0x24;
constexpr size_t S3M_TRACKER_VERSION_OFFSET = 0x28;
constexpr uint16_t S3M_TRACKER_VERSION = 0x1320;
constexpr size_t S3M_SAMPLE_FORMAT_OFFSET = 0x2A;
constexpr uint8_t S3M_UNSIGNED_SAMPLES = 2;
constexpr size_t S3M_SIGNATURE_OFFSET = 0x2C;
constexpr size_t S3M_GLOBAL_VOLUME_OFFSET = 0x30;
constexpr uint8_t S3M_GLOBAL_VOLUME = 64;
constexpr size_t S3M_SPEED_OFFSET = 0x31;
constexpr size_t S3M_TEMPO_OFFSET = 0x32;
constexpr size_t S3M_MASTER_VOLUME_OFFSET = 0x33;
constexpr uint8_t S3M_STEREO_FLAG = 0x80;
constexpr uint8_t S3M_MASTER_VOLUME = 48;
constexpr size_t S3M_CLICK_REMOVAL_OFFSET = 0x34;
constexpr uint8_t S3M_CLICK_REMOVAL = 16;
constexpr size_t S3M_PANNING_FLAG_OFFSET = 0x35;
constexpr uint8_t S3M_CHANNEL_PANNING = 0xFC;
constexpr size_t S3M_CHANNEL_SETTINGS_OFFSET = 0x40;
constexpr size_t S3M_CHANNEL_SETTING_COUNT = 32;
constexpr uint8_t S3M_UNUSED_CHANNEL = 0xFF;
constexpr size_t S3M_PANNING_SIZE = 32;
constexpr uint8_t S3M_PAN_SET = 0x20;
constexpr uint8_t S3M_PAN_LEFT = 3;
constexpr uint8_t S3M_PAN_RIGHT = 12;

constexpr size_t INSTRUMENT_SIZE = 0x50;
constexpr uint8_t INSTRUMENT_SAMPLE_TYPE = 1;
constexpr size_t INSTRUMENT_DOS_NAME_OFFSET = 1;
constexpr size_t INSTRUMENT_DOS_NAME_SIZE = 12;
constexpr size_t INSTRUMENT_MEMSEG_OFFSET = 0x0D;
constexpr size_t INSTRUMENT_LENGTH_OFFSET = 0x10;
constexpr size_t INSTRUMENT_LOOP_START_OFFSET = 0x14;
constexpr size_t INSTRUMENT_LOOP_END_OFFSET = 0x18;
constexpr size_t INSTRUMENT_VOLUME_OFFSET = 0x1C;
constexpr size_t INSTRUMENT_FLAGS_OFFSET = 0x1F;
constexpr uint8_t INSTRUMENT_LOOP_FLAG = 1;
constexpr size_t INSTRUMENT_C2SPD_OFFSET = 0x20;
constexpr size_t INSTRUMENT_NAME_OFFSET = 0x30;
constexpr size_t INSTRUMENT_SIGNATURE_OFFSET = 0x4C;
constexpr uint16_t MAX_NO_LOOP_WORDS = 2;
constexpr uint32_t MAX_NO_LOOP_SIZE = 4;
constexpr uint8_t SAMPLE_SIGN_BIT = 0x80;
constexpr uint8_t UNSIGNED_SILENCE = 0x80;

constexpr int CHANNEL_COUNT = 4;
constexpr int PATTERN_ROW_COUNT = 64;
constexpr size_t MAX_S3M_PATTERNS = 254;
constexpr uint16_t FRANKO_MENU_TEMPO = 37;
constexpr uint8_t AMIGA_CHANNEL_SETTINGS[CHANNEL_COUNT] = {0x00, 0x08, 0x09,
                                                           0x01};

constexpr uint16_t PERIOD_TABLE[] = {
    1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960, 906,
    856,  808,  762,  720,  678,  640,  604,  570,  538,  508,  480, 453,
    428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240, 226,
    214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120, 113,
    107,  101,  95,   90,   85,   80,   75,   71,   67,   63,   60,  56,
};
constexpr int PERIOD_COUNT = static_cast<int>(std::size(PERIOD_TABLE));
constexpr int SEMITONE_COUNT = 12;
constexpr int FIRST_OCTAVE = 2;

uint8_t periodToS3mNote(uint16_t period) {
  if (period == 0) {
    return S3M_NOTE_NONE;
  }
  int best = 0;
  int bestDistance = std::numeric_limits<int>::max();
  for (int i = 0; i < PERIOD_COUNT; i++) {
    const int distance =
        std::abs(static_cast<int>(PERIOD_TABLE[i]) - static_cast<int>(period));
    if (distance < bestDistance) {
      bestDistance = distance;
      best = i;
    }
  }
  int octave = best / SEMITONE_COUNT + FIRST_OCTAVE;
  int semitone = best % SEMITONE_COUNT;
  return static_cast<uint8_t>((octave << 4) | semitone);
}

struct AmosSample {
  uint32_t pcmOffset = 0;
  uint32_t length = 0;
  uint32_t loopStart = 0;
  uint16_t loopLength = 0;
  uint16_t volume = 0;
  char name[AMOS_NAME_SIZE + 1] = {};
};

struct RowEvent {
  uint8_t note = S3M_NOTE_NONE;
  uint8_t instrument = 0;
  uint8_t volume = S3M_VOLUME_NONE;
  uint8_t effect = 0;
  uint8_t effectParam = 0;
};

struct Pattern {
  RowEvent channels[CHANNEL_COUNT][PATTERN_ROW_COUNT];
};

std::vector<AmosSample> parseSamples(const std::vector<uint8_t> &music,
                                     size_t sampleInfoOffset) {
  std::vector<AmosSample> samples;
  if (sampleInfoOffset + SAMPLES_TABLE_OFFSET > music.size()) {
    return samples;
  }
  binary::BigEndianReader reader(music);
  uint16_t count = reader.readUint16(sampleInfoOffset);
  if (count == 0 || count > MAX_SAMPLE_COUNT) {
    return samples;
  }
  size_t tableOffset = sampleInfoOffset + SAMPLES_TABLE_OFFSET;
  for (uint16_t i = 0; i < count; i++) {
    size_t off = tableOffset + static_cast<size_t>(i) * SAMPLE_DESCRIPTOR_SIZE;
    if (off + SAMPLE_DESCRIPTOR_SIZE > music.size()) {
      break;
    }
    AmosSample sample;
    sample.pcmOffset = reader.readUint32(off);
    uint32_t loopPos = reader.readUint32(off + SAMPLE_LOOP_POINTER_OFFSET);
    sample.loopLength = reader.readUint16(off + SAMPLE_LOOP_LENGTH_OFFSET);
    sample.volume = reader.readUint16(off + SAMPLE_VOLUME_OFFSET);
    sample.length =
        static_cast<uint32_t>(reader.readUint16(off + SAMPLE_LENGTH_OFFSET)) *
        2;
    sample.loopStart =
        (loopPos > sample.pcmOffset) ? (loopPos - sample.pcmOffset) : 0;
    std::memcpy(sample.name, music.data() + off + SAMPLE_NAME_OFFSET,
                AMOS_NAME_SIZE);
    samples.push_back(sample);
  }
  return samples;
}

struct SongInfo {
  uint16_t speed = DEFAULT_SONG_SPEED;
  std::vector<uint16_t> orders[CHANNEL_COUNT];
  char name[AMOS_NAME_SIZE + 1] = {};
};

SongInfo parseSong(const std::vector<uint8_t> &music, size_t songOffset) {
  SongInfo info;
  if (songOffset + SONGS_TABLE_OFFSET + SONG_POINTER_SIZE > music.size()) {
    return info;
  }

  binary::BigEndianReader reader(music);
  const uint32_t songDataOffset =
      reader.readUint32(songOffset + SONGS_TABLE_OFFSET);
  size_t songBase = songOffset + songDataOffset;
  if (songBase + SONG_HEADER_SIZE > music.size()) {
    return info;
  }

  uint16_t channelOffsets[CHANNEL_COUNT];
  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    channelOffsets[channel] = reader.readUint16(songBase + channel * 2);
  }
  info.speed = reader.readUint16(songBase + SONG_SPEED_OFFSET);
  std::memcpy(info.name, music.data() + songBase + SONG_NAME_OFFSET,
              AMOS_NAME_SIZE);

  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    size_t pos = songBase + channelOffsets[channel];
    while (pos + 2 <= music.size()) {
      uint16_t val = reader.readUint16(pos);
      if (val & AMOS_ORDER_END_FLAG) {
        break;
      }
      info.orders[channel].push_back(val);
      pos += 2;
    }
  }
  return info;
}

struct TrackInfo {
  uint16_t numberOfSteps = 0;
  std::vector<uint16_t> offsets;
  size_t trackDataBase = 0;
};

TrackInfo parseTrackData(const std::vector<uint8_t> &music,
                         size_t trackOffset) {
  TrackInfo info;
  info.trackDataBase = trackOffset;
  if (trackOffset + TRACKS_TABLE_OFFSET > music.size()) {
    return info;
  }
  binary::BigEndianReader reader(music);
  info.numberOfSteps = reader.readUint16(trackOffset);
  size_t numberOfOffsets =
      static_cast<size_t>(info.numberOfSteps) * CHANNEL_COUNT;
  for (size_t i = 0; i < numberOfOffsets; i++) {
    size_t pos = trackOffset + TRACKS_TABLE_OFFSET + i * 2;
    if (pos + 2 > music.size()) {
      break;
    }
    info.offsets.push_back(reader.readUint16(pos));
  }
  return info;
}

struct DecodedPattern {
  Pattern pattern;
  int endRow = PATTERN_ROW_COUNT;
};

void decodeChannel(Pattern &pattern, int channel,
                   const std::vector<uint8_t> &music, const TrackInfo &track,
                   uint16_t stepIndex, int &channelEndRow) {
  size_t tableIndex = static_cast<size_t>(stepIndex) * CHANNEL_COUNT + channel;
  if (tableIndex >= track.offsets.size()) {
    return;
  }

  if (track.offsets[tableIndex] == 0) {
    return;
  }

  binary::BigEndianReader reader(music);
  size_t pos = track.trackDataBase + track.offsets[tableIndex];
  int row = 0;
  uint8_t currentSample = 0;
  uint8_t currentVolume = S3M_VOLUME_NONE;
  uint8_t pendingEffect = 0;
  uint8_t pendingParam = 0;
  bool noteSet = false;
  uint16_t notePeriod = 0;

  while (pos + 2 <= music.size() && row < PATTERN_ROW_COUNT) {
    uint16_t word = reader.readUint16(pos);
    pos += 2;
    uint8_t command = word >> 8;
    uint8_t parameter = word & 0xFF;

    switch (command) {
    case AMOS_COMMAND_DELAY: {
      auto &event = pattern.channels[channel][row];
      if (noteSet) {
        event.note = periodToS3mNote(notePeriod);
        event.instrument = currentSample;
        noteSet = false;
      }
      if (currentVolume != S3M_VOLUME_NONE) {
        event.volume = currentVolume;
        currentVolume = S3M_VOLUME_NONE;
      }
      if (pendingEffect != 0) {
        event.effect = pendingEffect;
        event.effectParam = pendingParam;
        pendingEffect = 0;
        pendingParam = 0;
      }
      row += parameter;
      break;
    }
    case AMOS_COMMAND_END:
      if (row < PATTERN_ROW_COUNT) {
        pattern.channels[channel][row].note = S3M_NOTE_OFF;
        pattern.channels[channel][row].instrument = 0;
      }
      channelEndRow = row;
      return;
    case AMOS_COMMAND_SET_VOLUME:
      currentVolume = std::min(parameter, S3M_MAX_VOLUME);
      break;
    case AMOS_COMMAND_SET_SAMPLE:
      currentSample = parameter + 1;
      break;
    case AMOS_COMMAND_STOP_EFFECT:
      pendingEffect = 0;
      pendingParam = 0;
      break;
    case AMOS_COMMAND_SET_TEMPO:
      if (parameter > 0) {
        pendingEffect = S3M_EFFECT_SPEED;
        pendingParam = parameter;
      }
      break;
    case AMOS_COMMAND_ARPEGGIO:
      pendingEffect = S3M_EFFECT_ARPEGGIO;
      pendingParam = parameter;
      break;
    case AMOS_COMMAND_PORTAMENTO:
      pendingEffect = S3M_EFFECT_TONE_PORTA;
      pendingParam = parameter;
      break;
    case AMOS_COMMAND_VIBRATO:
      pendingEffect = S3M_EFFECT_VIBRATO;
      pendingParam = parameter;
      break;
    case AMOS_COMMAND_VOLUME_SLIDE:
      pendingEffect = S3M_EFFECT_VOLUME_SLIDE;
      pendingParam = parameter;
      break;
    case AMOS_COMMAND_SLIDE_UP:
      pendingEffect = S3M_EFFECT_PORTA_UP;
      pendingParam = parameter;
      break;
    case AMOS_COMMAND_SLIDE_DOWN:
      pendingEffect = S3M_EFFECT_PORTA_DOWN;
      pendingParam = parameter;
      break;
    case AMOS_COMMAND_POSITION_JUMP:
      pendingEffect = S3M_EFFECT_POSITION_JUMP;
      pendingParam = parameter;
      break;
    default:
      if ((command & AMOS_COMMAND_FLAG) == 0 && word != 0) {
        notePeriod = word & AMOS_PERIOD_MASK;
        noteSet = true;
      }
      break;
    }
  }

  if (channelEndRow == PATTERN_ROW_COUNT && row < PATTERN_ROW_COUNT &&
      (noteSet || pendingEffect != 0 || currentVolume != S3M_VOLUME_NONE)) {
    auto &event = pattern.channels[channel][row];
    if (noteSet) {
      event.note = periodToS3mNote(notePeriod);
      event.instrument = currentSample;
    }
    if (currentVolume != S3M_VOLUME_NONE) {
      event.volume = currentVolume;
    }
    if (pendingEffect != 0) {
      event.effect = pendingEffect;
      event.effectParam = pendingParam;
    }
  }
}

DecodedPattern decodePattern(const std::vector<uint8_t> &music,
                             const TrackInfo &track,
                             const uint16_t stepIndices[CHANNEL_COUNT]) {
  Pattern pattern;

  int channelEndRows[CHANNEL_COUNT];
  std::fill(std::begin(channelEndRows), std::end(channelEndRows),
            PATTERN_ROW_COUNT);
  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    decodeChannel(pattern, channel, music, track, stepIndices[channel],
                  channelEndRows[channel]);
  }

  int endRow = PATTERN_ROW_COUNT;
  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    if (channelEndRows[channel] < endRow) {
      endRow = channelEndRows[channel];
    }
  }
  if (endRow < PATTERN_ROW_COUNT) {
    for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
      if (pattern.channels[channel][endRow].effect == 0) {
        pattern.channels[channel][endRow].effect = S3M_EFFECT_PATTERN_BREAK;
        pattern.channels[channel][endRow].effectParam = 0;
        break;
      }
    }
  }

  return {pattern, endRow};
}

std::vector<uint8_t> packPattern(const Pattern &pattern) {
  std::vector<uint8_t> packed;
  binary::pushLittleEndian16(packed, 0);

  for (int row = 0; row < PATTERN_ROW_COUNT; row++) {
    for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
      const auto &event = pattern.channels[channel][row];
      uint8_t what = 0;
      if (event.note != S3M_NOTE_NONE || event.instrument != 0) {
        what |= S3M_PACKED_NOTE;
      }
      if (event.volume != S3M_VOLUME_NONE) {
        what |= S3M_PACKED_VOLUME;
      }
      if (event.effect != 0) {
        what |= S3M_PACKED_EFFECT;
      }
      if (what == 0) {
        continue;
      }
      what |= static_cast<uint8_t>(channel);
      packed.push_back(what);
      if (what & S3M_PACKED_NOTE) {
        packed.push_back(event.note);
        packed.push_back(event.instrument);
      }
      if (what & S3M_PACKED_VOLUME) {
        packed.push_back(event.volume);
      }
      if (what & S3M_PACKED_EFFECT) {
        packed.push_back(event.effect);
        packed.push_back(event.effectParam);
      }
    }
    packed.push_back(0);
  }

  binary::writeLittleEndian16(packed, 0, static_cast<uint16_t>(packed.size()));
  return packed;
}

struct SpeedTempo {
  uint8_t speed = 0;
  uint8_t tempo = 0;
  bool hasTempo = false;
};

SpeedTempo amosTempoToS3m(uint8_t amosTempo) {
  if (amosTempo == 0) {
    return {DEFAULT_S3M_SPEED, DEFAULT_S3M_TEMPO, false};
  }
  const int speed = std::clamp((100 + amosTempo / 2) / amosTempo, MIN_S3M_SPEED,
                               MAX_S3M_SPEED);
  const int tempo =
      std::clamp((5 * speed * amosTempo + 2) / 4, MIN_S3M_TEMPO, MAX_S3M_TEMPO);
  return {static_cast<uint8_t>(speed), static_cast<uint8_t>(tempo), true};
}

void fixSpeedEffects(Pattern &pattern) {
  for (int row = 0; row < PATTERN_ROW_COUNT; row++) {
    for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
      auto &event = pattern.channels[channel][row];
      if (event.effect != S3M_EFFECT_SPEED) {
        continue;
      }
      auto speedTempo = amosTempoToS3m(event.effectParam);
      event.effectParam = speedTempo.speed;
      if (speedTempo.hasTempo) {
        for (int other = 0; other < CHANNEL_COUNT; other++) {
          if (other != channel && pattern.channels[other][row].effect == 0) {
            pattern.channels[other][row].effect = S3M_EFFECT_TEMPO;
            pattern.channels[other][row].effectParam = speedTempo.tempo;
            break;
          }
        }
      }
    }
  }
}

std::vector<Pattern> decodeAllPatterns(const std::vector<uint8_t> &music,
                                       const TrackInfo &track,
                                       const SongInfo &song,
                                       std::vector<uint8_t> &orderList) {
  std::vector<Pattern> patterns;

  size_t songLength = 0;
  for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
    songLength = std::max(songLength, song.orders[channel].size());
  }

  for (size_t pos = 0; pos < songLength; pos++) {
    uint16_t steps[CHANNEL_COUNT];
    for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
      steps[channel] =
          pos < song.orders[channel].size() ? song.orders[channel][pos] : 0;
    }

    auto decoded = decodePattern(music, track, steps);
    fixSpeedEffects(decoded.pattern);
    bool found = false;
    for (size_t i = 0; i < patterns.size(); i++) {
      if (std::memcmp(&patterns[i], &decoded.pattern, sizeof(Pattern)) == 0) {
        orderList.push_back(static_cast<uint8_t>(i));
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
      patterns.push_back(decoded.pattern);
    }
  }

  return patterns;
}

void writeS3mHeader(std::vector<uint8_t> &s3m, const SongInfo &song,
                    uint16_t ordNum, uint16_t insNum, uint16_t patNum,
                    uint8_t speed, uint8_t tempo) {
  s3m.resize(S3M_HEADER_SIZE, 0);

  size_t nameLength = std::min(std::strlen(song.name), S3M_SONG_NAME_SIZE);
  std::memcpy(s3m.data(), song.name, nameLength);
  s3m[S3M_EOF_OFFSET] = S3M_EOF_MARKER;
  s3m[S3M_TYPE_OFFSET] = S3M_MODULE_TYPE;
  binary::writeLittleEndian16(s3m, S3M_ORDER_COUNT_OFFSET, ordNum);
  binary::writeLittleEndian16(s3m, S3M_INSTRUMENT_COUNT_OFFSET, insNum);
  binary::writeLittleEndian16(s3m, S3M_PATTERN_COUNT_OFFSET, patNum);
  binary::writeLittleEndian16(s3m, S3M_TRACKER_VERSION_OFFSET,
                              S3M_TRACKER_VERSION);
  s3m[S3M_SAMPLE_FORMAT_OFFSET] = S3M_UNSIGNED_SAMPLES;
  s3m[S3M_SIGNATURE_OFFSET + 0] = 'S';
  s3m[S3M_SIGNATURE_OFFSET + 1] = 'C';
  s3m[S3M_SIGNATURE_OFFSET + 2] = 'R';
  s3m[S3M_SIGNATURE_OFFSET + 3] = 'M';
  s3m[S3M_GLOBAL_VOLUME_OFFSET] = S3M_GLOBAL_VOLUME;
  s3m[S3M_SPEED_OFFSET] = speed;
  s3m[S3M_TEMPO_OFFSET] = tempo;
  s3m[S3M_MASTER_VOLUME_OFFSET] = S3M_STEREO_FLAG | S3M_MASTER_VOLUME;
  s3m[S3M_CLICK_REMOVAL_OFFSET] = S3M_CLICK_REMOVAL;
  s3m[S3M_PANNING_FLAG_OFFSET] = S3M_CHANNEL_PANNING;

  for (size_t i = 0; i < S3M_CHANNEL_SETTING_COUNT; i++) {
    s3m[S3M_CHANNEL_SETTINGS_OFFSET + i] =
        i < static_cast<size_t>(CHANNEL_COUNT) ? AMIGA_CHANNEL_SETTINGS[i]
                                               : S3M_UNUSED_CHANNEL;
  }
}

void writeInstrument(std::vector<uint8_t> &s3m, size_t instrumentOffset,
                     const AmosSample &sample, int index) {
  s3m.resize(instrumentOffset + INSTRUMENT_SIZE, 0);
  s3m[instrumentOffset] = INSTRUMENT_SAMPLE_TYPE;
  char dosName[INSTRUMENT_DOS_NAME_SIZE + 1] = {};
  snprintf(dosName, sizeof(dosName), "SAMPLE%02d.RAW", index + 1);
  std::memcpy(s3m.data() + instrumentOffset + INSTRUMENT_DOS_NAME_OFFSET,
              dosName, INSTRUMENT_DOS_NAME_SIZE);

  uint32_t sampleLength = sample.length;
  uint32_t loopStart = sample.loopStart;
  uint32_t loopEnd = loopStart + static_cast<uint32_t>(sample.loopLength) * 2;
  if (loopEnd > sampleLength) {
    loopEnd = sampleLength;
  }
  bool hasLoop = sample.loopLength > MAX_NO_LOOP_WORDS &&
                 loopEnd > loopStart + MAX_NO_LOOP_SIZE;

  binary::writeLittleEndian32(s3m, instrumentOffset + INSTRUMENT_LENGTH_OFFSET,
                              sampleLength);
  binary::writeLittleEndian32(
      s3m, instrumentOffset + INSTRUMENT_LOOP_START_OFFSET, loopStart);
  binary::writeLittleEndian32(
      s3m, instrumentOffset + INSTRUMENT_LOOP_END_OFFSET, loopEnd);

  uint8_t volume =
      static_cast<uint8_t>(std::min<uint16_t>(sample.volume, S3M_MAX_VOLUME));
  s3m[instrumentOffset + INSTRUMENT_VOLUME_OFFSET] = volume;

  if (hasLoop) {
    s3m[instrumentOffset + INSTRUMENT_FLAGS_OFFSET] = INSTRUMENT_LOOP_FLAG;
  }

  binary::writeLittleEndian32(s3m, instrumentOffset + INSTRUMENT_C2SPD_OFFSET,
                              gameData::audio::DEFAULT_SAMPLE_RATE);

  std::memcpy(s3m.data() + instrumentOffset + INSTRUMENT_NAME_OFFSET,
              sample.name, AMOS_NAME_SIZE);

  const size_t signature = instrumentOffset + INSTRUMENT_SIGNATURE_OFFSET;
  s3m[signature + 0] = 'S';
  s3m[signature + 1] = 'C';
  s3m[signature + 2] = 'R';
  s3m[signature + 3] = 'S';
}

} // namespace

std::vector<uint8_t> convert(const std::vector<uint8_t> &abkData,
                             uint16_t initialAmosTempo) {
  if (abkData.size() < ABK_HEADER_SIZE || abkData[0] != 'A' ||
      abkData[1] != 'm' || abkData[2] != 'B' || abkData[3] != 'k') {
    throw std::runtime_error("Not a valid AmBk file");
  }

  const std::vector<uint8_t> music(abkData.begin() + ABK_HEADER_SIZE,
                                   abkData.end());
  if (music.size() < MUSIC_HEADER_SIZE) {
    throw std::runtime_error("Music data too small");
  }

  binary::BigEndianReader reader(music);
  uint32_t sampleInfoOffset = reader.readUint32(MUSIC_SAMPLES_POINTER_OFFSET);
  uint32_t songOffset = reader.readUint32(MUSIC_SONGS_POINTER_OFFSET);
  uint32_t trackOffset = reader.readUint32(MUSIC_TRACKS_POINTER_OFFSET);

  auto samples = parseSamples(music, sampleInfoOffset);
  auto song = parseSong(music, songOffset);
  auto track = parseTrackData(music, trackOffset);

  std::vector<uint8_t> orderList;
  auto patterns = decodeAllPatterns(music, track, song, orderList);
  if (patterns.empty()) {
    throw std::runtime_error("Empty song");
  }

  uint16_t ordNum = static_cast<uint16_t>(orderList.size());
  if (ordNum % 2 != 0) {
    orderList.push_back(S3M_ORDER_END);
    ordNum++;
  }
  uint16_t insNum = static_cast<uint16_t>(samples.size());
  uint16_t patNum = static_cast<uint16_t>(patterns.size());

  uint16_t amosTempo = initialAmosTempo;
  if (amosTempo == 0) {
    amosTempo =
        std::strncmp(song.name, "e1", 2) == 0 ? FRANKO_MENU_TEMPO : song.speed;
  }

  const auto initial =
      amosTempoToS3m(static_cast<uint8_t>(std::min(amosTempo, MAX_AMOS_TEMPO)));

  std::vector<uint8_t> s3m;
  writeS3mHeader(s3m, song, ordNum, insNum, patNum, initial.speed,
                 initial.tempo);
  s3m.insert(s3m.end(), orderList.begin(), orderList.end());

  size_t instrumentTableOffset = s3m.size();
  for (uint16_t i = 0; i < insNum; i++) {
    binary::pushLittleEndian16(s3m, 0);
  }

  size_t patternTableOffset = s3m.size();
  for (uint16_t i = 0; i < patNum; i++) {
    binary::pushLittleEndian16(s3m, 0);
  }

  uint8_t panning[S3M_PANNING_SIZE] = {};
  panning[0] = S3M_PAN_SET | S3M_PAN_LEFT;
  panning[1] = S3M_PAN_SET | S3M_PAN_RIGHT;
  panning[2] = S3M_PAN_SET | S3M_PAN_RIGHT;
  panning[3] = S3M_PAN_SET | S3M_PAN_LEFT;
  s3m.insert(s3m.end(), panning, panning + S3M_PANNING_SIZE);

  std::vector<size_t> instrumentOffsets(insNum);
  for (uint16_t i = 0; i < insNum; i++) {
    binary::padTo16(s3m);
    instrumentOffsets[i] = s3m.size();
    binary::writeLittleEndian16(
        s3m, instrumentTableOffset + i * 2,
        static_cast<uint16_t>(instrumentOffsets[i] / S3M_PARAGRAPH_SIZE));
    writeInstrument(s3m, s3m.size(), samples[i], i);
  }

  for (uint16_t i = 0; i < patNum; i++) {
    binary::padTo16(s3m);
    binary::writeLittleEndian16(
        s3m, patternTableOffset + i * 2,
        static_cast<uint16_t>(s3m.size() / S3M_PARAGRAPH_SIZE));

    auto packed = packPattern(patterns[i]);
    s3m.insert(s3m.end(), packed.begin(), packed.end());
  }

  for (uint16_t i = 0; i < insNum; i++) {
    binary::padTo16(s3m);
    uint32_t parapointer =
        static_cast<uint32_t>(s3m.size() / S3M_PARAGRAPH_SIZE);
    s3m[instrumentOffsets[i] + INSTRUMENT_MEMSEG_OFFSET] =
        static_cast<uint8_t>(parapointer >> 16);
    binary::writeLittleEndian16(
        s3m, instrumentOffsets[i] + INSTRUMENT_MEMSEG_OFFSET + 1,
        static_cast<uint16_t>(parapointer));

    const size_t pcmOffset = samples[i].pcmOffset;
    const size_t length = samples[i].length;
    for (size_t j = 0; j < length; j++) {
      const size_t pos = static_cast<size_t>(sampleInfoOffset) + pcmOffset + j;
      if (pos < music.size()) {
        s3m.push_back(static_cast<uint8_t>(music[pos] ^ SAMPLE_SIGN_BIT));
      } else {
        s3m.push_back(UNSIGNED_SILENCE);
      }
    }
  }

  return s3m;
}

} // namespace openfranko::lib::converter::abkToS3m
