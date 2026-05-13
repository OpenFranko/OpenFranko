#include "audioExtractor.h"
#include "../../decompressor/helpers/helpers.h"
#include <cstring>

namespace openfranko::lib::converter::audioExtractor {

namespace helpers = decompressor::helpers;
using helpers::pushBigEndian16;
using helpers::pushBigEndian32;
using helpers::pushLittleEndian16;
using helpers::pushLittleEndian32;

namespace {

std::vector<uint8_t> pcmToWav(const int8_t *pcm, uint32_t numSamples,
                              uint32_t sampleRate) {
  uint32_t dataLen = numSamples * 2;
  uint32_t fileSize = 36 + dataLen;

  std::vector<uint8_t> buf;
  buf.reserve(44 + dataLen);

  buf.push_back('R');
  buf.push_back('I');
  buf.push_back('F');
  buf.push_back('F');
  pushLittleEndian32(buf, fileSize);
  buf.push_back('W');
  buf.push_back('A');
  buf.push_back('V');
  buf.push_back('E');

  buf.push_back('f');
  buf.push_back('m');
  buf.push_back('t');
  buf.push_back(' ');
  pushLittleEndian32(buf, 16);
  pushLittleEndian16(buf, 1);
  pushLittleEndian16(buf, 1);
  pushLittleEndian32(buf, sampleRate);
  pushLittleEndian32(buf, sampleRate * 2);
  pushLittleEndian16(buf, 2);
  pushLittleEndian16(buf, 16);

  buf.push_back('d');
  buf.push_back('a');
  buf.push_back('t');
  buf.push_back('a');
  pushLittleEndian32(buf, dataLen);

  for (uint32_t i = 0; i < numSamples; i++) {
    int16_t s = static_cast<int16_t>(pcm[i]) * 256;
    pushLittleEndian16(buf, static_cast<uint16_t>(s));
  }

  return buf;
}

} // namespace

std::vector<ExtractedAudio>
extractStandaloneSamBank(const std::vector<uint8_t> &data,
                         const std::string &fileId) {
  std::vector<ExtractedAudio> results;

  if (data.size() < 6) {
    return results;
  }

  uint16_t maxSample = helpers::readUint16BigEndian(data, 0);
  if (maxSample == 0 || maxSample > 100) {
    return results;
  }

  size_t tableSize = 2 + static_cast<size_t>(maxSample) * 4;
  if (data.size() < tableSize) {
    return results;
  }

  for (uint16_t i = 0; i < maxSample; i++) {
    uint32_t sampleOffset = helpers::readUint32BigEndian(data, 2 + i * 4);
    if (sampleOffset == 0) {
      continue;
    }

    if (sampleOffset + 14 >= data.size()) {
      continue;
    }

    uint16_t freq = helpers::readUint16BigEndian(data, sampleOffset + 8);
    uint32_t length = helpers::readUint32BigEndian(data, sampleOffset + 10);
    size_t pcmStart = sampleOffset + 14;

    if (freq == 0) {
      freq = 8287;
    }
    if (pcmStart + length > data.size()) {
      length = static_cast<uint32_t>(data.size() - pcmStart);
    }
    if (length == 0) {
      continue;
    }

    auto wav = pcmToWav(
        reinterpret_cast<const int8_t *>(data.data() + pcmStart), length, freq);
    results.push_back({fileId + "_sam" + std::to_string(i + 1) + "_" +
                           std::to_string(freq) + "Hz.wav",
                       std::move(wav)});
  }

  return results;
}

std::vector<ExtractedAudio>
extractEmbeddedSamBank(const std::vector<uint8_t> &data,
                       const std::string &fileId) {
  std::vector<ExtractedAudio> results;

  if (data.size() < 12) {
    return results;
  }

  uint32_t sbOff = helpers::readUint32BigEndian(data, 8);
  if (sbOff == 0 || sbOff + 6 >= data.size()) {
    return results;
  }

  uint16_t count = helpers::readUint16BigEndian(data, sbOff);
  if (count == 0 || count > 50) {
    return results;
  }

  size_t tableEnd = sbOff + 2 + static_cast<size_t>(count) * 4;
  if (tableEnd >= data.size()) {
    return results;
  }

  uint32_t prev = 0;
  for (uint16_t i = 0; i < count; i++) {
    uint32_t v = helpers::readUint32BigEndian(data, sbOff + 2 + i * 4);
    if (v <= prev || v >= data.size()) {
      return results;
    }
    prev = v;
  }

  for (uint16_t i = 0; i < count; i++) {
    size_t sampleOffset =
        sbOff + helpers::readUint32BigEndian(data, sbOff + 2 + i * 4);

    if (sampleOffset + 14 >= data.size()) {
      continue;
    }

    uint16_t freq = helpers::readUint16BigEndian(data, sampleOffset + 8);
    uint32_t length = helpers::readUint32BigEndian(data, sampleOffset + 10);
    size_t pcmStart = sampleOffset + 14;

    if (freq == 0) {
      freq = 8287;
    }
    if (pcmStart + length > data.size()) {
      length = static_cast<uint32_t>(data.size() - pcmStart);
    }
    if (length == 0) {
      continue;
    }

    auto wav = pcmToWav(
        reinterpret_cast<const int8_t *>(data.data() + pcmStart), length, freq);

    results.push_back({fileId + "_sam" + std::to_string(i + 1) + "_" +
                           std::to_string(freq) + "Hz.wav",
                       std::move(wav)});
  }

  return results;
}

void patchMissingSpeedCommand(std::vector<uint8_t> &music) {
  if (music.size() < 12) {
    return;
  }
  uint32_t trackOff = helpers::readUint32BigEndian(music, 8);
  if (trackOff + 2 > music.size()) {
    return;
  }
  uint16_t numSteps = helpers::readUint16BigEndian(music, trackOff);
  size_t tableBytes = 2 + static_cast<size_t>(numSteps) * 4 * 2;
  size_t patternStart = trackOff + tableBytes;
  if (patternStart + 2 > music.size()) {
    return;
  }
  uint8_t hi = music[patternStart];
  if (hi == 0x88) {
    return;
  }
  music[patternStart] = 0x88;
  music[patternStart + 1] = 0x21;
}

ExtractedAudio wrapMusicBank(const std::vector<uint8_t> &data,
                             const std::string &fileId) {
  auto music = data;
  patchMissingSpeedCommand(music);

  std::vector<uint8_t> abk;
  abk.reserve(20 + music.size());

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

  return {fileId + ".abk", std::move(abk)};
}

} // namespace openfranko::lib::converter::audioExtractor
