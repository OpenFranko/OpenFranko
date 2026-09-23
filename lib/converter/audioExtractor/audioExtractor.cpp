#include "audioExtractor.h"
#include "../../helpers/helpers.h"
#include "../gameData/gameData.h"
#include <cstring>

namespace openfranko::lib::converter::audioExtractor {

using helpers::pushBigEndian16;
using helpers::pushBigEndian32;
using helpers::pushLittleEndian16;
using helpers::pushLittleEndian32;

namespace {

constexpr size_t SAMPLE_HEADER_SIZE = 14;

std::vector<uint8_t> pcmToWav(const int8_t *pcm, uint32_t numSamples,
                              uint32_t sampleRate) {
  uint32_t dataLen = numSamples;
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
  pushLittleEndian32(buf, sampleRate);
  pushLittleEndian16(buf, 1);
  pushLittleEndian16(buf, 8);

  buf.push_back('d');
  buf.push_back('a');
  buf.push_back('t');
  buf.push_back('a');
  pushLittleEndian32(buf, dataLen);

  // WAV 8-bit uses unsigned (128 = silence), Amiga uses signed (0 = silence)
  for (uint32_t i = 0; i < numSamples; i++) {
    buf.push_back(static_cast<uint8_t>(pcm[i] + 128));
  }

  return buf;
}

bool appendSample(const std::vector<uint8_t> &data, size_t sampleOffset,
                  uint16_t sampleNumber, const std::string &fileId,
                  std::vector<ExtractedAudio> &results) {
  if (sampleOffset + SAMPLE_HEADER_SIZE >= data.size()) {
    return false;
  }

  helpers::BigEndianReader reader(data);
  uint16_t freq = reader.readUint16(sampleOffset + 8);
  uint32_t length = reader.readUint32(sampleOffset + 10);
  size_t pcmStart = sampleOffset + SAMPLE_HEADER_SIZE;

  if (freq == 0) {
    freq = gameData::audio::DEFAULT_SAMPLE_RATE;
  }
  if (pcmStart + length > data.size()) {
    length = static_cast<uint32_t>(data.size() - pcmStart);
  }
  if (length == 0) {
    return false;
  }

  auto wav = pcmToWav(reinterpret_cast<const int8_t *>(data.data() + pcmStart),
                      length, freq);
  results.push_back({fileId + "_sam" + std::to_string(sampleNumber) + "_" +
                         std::to_string(freq) + "Hz.wav",
                     std::move(wav)});
  return true;
}

} // namespace

std::vector<ExtractedAudio>
extractStandaloneSamBank(const std::vector<uint8_t> &data,
                         const std::string &fileId) {
  std::vector<ExtractedAudio> results;

  if (data.size() < 6) {
    return results;
  }

  helpers::BigEndianReader reader(data);
  uint16_t maxSample = reader.readUint16(0);
  if (maxSample == 0 || maxSample > 100) {
    return results;
  }

  size_t tableSize = 2 + static_cast<size_t>(maxSample) * 4;
  if (data.size() < tableSize) {
    return results;
  }

  for (uint16_t i = 0; i < maxSample; i++) {
    uint32_t sampleOffset = reader.readUint32(2 + i * 4);
    if (sampleOffset == 0) {
      continue;
    }

    appendSample(data, sampleOffset, i + 1, fileId, results);
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

  helpers::BigEndianReader reader(data);
  const size_t sbOff = reader.readUint32(8);
  if (sbOff == 0 || sbOff + 6 >= data.size()) {
    return results;
  }

  uint16_t count = reader.readUint16(sbOff);
  if (count == 0 || count > 50) {
    return results;
  }

  size_t tableEnd = sbOff + 2 + static_cast<size_t>(count) * 4;
  if (tableEnd >= data.size()) {
    return results;
  }

  uint32_t prev = 0;
  for (uint16_t i = 0; i < count; i++) {
    const uint32_t v = reader.readUint32(sbOff + 2 + i * 4);
    if (v <= prev || sbOff + v >= data.size()) {
      return results;
    }
    prev = v;
  }

  for (uint16_t i = 0; i < count; i++) {
    size_t sampleOffset = sbOff + reader.readUint32(sbOff + 2 + i * 4);

    appendSample(data, sampleOffset, i + 1, fileId, results);
  }

  return results;
}

ExtractedAudio wrapMusicBank(const std::vector<uint8_t> &data,
                             const std::string &fileId) {
  std::vector<uint8_t> abk;
  abk.reserve(20 + data.size());

  abk.push_back('A');
  abk.push_back('m');
  abk.push_back('B');
  abk.push_back('k');
  pushBigEndian16(abk, 3);
  pushBigEndian16(abk, 0);
  pushBigEndian32(abk, static_cast<uint32_t>(data.size() + 8) | 0x80000000u);

  const char *bankName = "Music   ";
  abk.insert(abk.end(), bankName, bankName + 8);
  abk.insert(abk.end(), data.begin(), data.end());

  return {fileId + ".abk", std::move(abk)};
}

} // namespace openfranko::lib::converter::audioExtractor
