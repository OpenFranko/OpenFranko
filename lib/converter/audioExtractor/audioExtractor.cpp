#include "audioExtractor.h"
#include "../../binary/binary.h"
#include "../gameData/gameData.h"

namespace openfranko::lib::converter::audioExtractor {

namespace {

constexpr size_t SAMPLE_HEADER_SIZE = 14;
constexpr size_t SAMPLE_FREQUENCY_OFFSET = 8;
constexpr size_t SAMPLE_LENGTH_OFFSET = 10;

constexpr size_t SAM_BANK_TABLE_OFFSET = 2;
constexpr size_t SAMPLE_POINTER_SIZE = 4;
constexpr uint16_t MAX_STANDALONE_SAMPLE_COUNT = 100;
constexpr uint16_t MAX_EMBEDDED_SAMPLE_COUNT = 50;

constexpr size_t SPRITE_BANK_HEADER_SIZE = 12;
constexpr size_t SAM_BANK_POINTER_OFFSET = 8;

constexpr uint32_t WAV_HEADER_SIZE = 44;
constexpr uint32_t WAV_CHUNK_HEADER_SIZE = 8;
constexpr uint32_t WAV_FORMAT_CHUNK_SIZE = 16;
constexpr uint16_t WAV_PCM_FORMAT = 1;
constexpr uint16_t WAV_CHANNEL_COUNT = 1;
constexpr uint16_t WAV_BLOCK_ALIGN = 1;
constexpr uint16_t WAV_BITS_PER_SAMPLE = 8;
constexpr int WAV_SILENCE = 128;

constexpr size_t ABK_HEADER_SIZE = 20;
constexpr uint16_t ABK_MUSIC_BANK_NUMBER = 3;
constexpr uint32_t ABK_CHIP_MEMORY_FLAG = 0x80000000u;
constexpr size_t ABK_BANK_NAME_SIZE = 8;

std::vector<uint8_t> pcmToWav(const int8_t *pcm, uint32_t numberOfSamples,
                              uint32_t sampleRate) {
  uint32_t dataLength = numberOfSamples;
  uint32_t fileSize = WAV_HEADER_SIZE - WAV_CHUNK_HEADER_SIZE + dataLength;

  std::vector<uint8_t> buf;
  buf.reserve(WAV_HEADER_SIZE + dataLength);

  buf.push_back('R');
  buf.push_back('I');
  buf.push_back('F');
  buf.push_back('F');
  binary::pushLittleEndian32(buf, fileSize);
  buf.push_back('W');
  buf.push_back('A');
  buf.push_back('V');
  buf.push_back('E');

  buf.push_back('f');
  buf.push_back('m');
  buf.push_back('t');
  buf.push_back(' ');
  binary::pushLittleEndian32(buf, WAV_FORMAT_CHUNK_SIZE);
  binary::pushLittleEndian16(buf, WAV_PCM_FORMAT);
  binary::pushLittleEndian16(buf, WAV_CHANNEL_COUNT);
  binary::pushLittleEndian32(buf, sampleRate);
  binary::pushLittleEndian32(buf, sampleRate * WAV_BLOCK_ALIGN);
  binary::pushLittleEndian16(buf, WAV_BLOCK_ALIGN);
  binary::pushLittleEndian16(buf, WAV_BITS_PER_SAMPLE);

  buf.push_back('d');
  buf.push_back('a');
  buf.push_back('t');
  buf.push_back('a');
  binary::pushLittleEndian32(buf, dataLength);

  for (uint32_t i = 0; i < numberOfSamples; i++) {
    buf.push_back(static_cast<uint8_t>(pcm[i] + WAV_SILENCE));
  }

  return buf;
}

bool appendSample(const std::vector<uint8_t> &data, size_t sampleOffset,
                  uint16_t sampleNumber, const std::string &fileId,
                  std::vector<ExtractedAudio> &results) {
  if (sampleOffset + SAMPLE_HEADER_SIZE >= data.size()) {
    return false;
  }

  binary::BigEndianReader reader(data);
  uint16_t frequency =
      reader.readUint16(sampleOffset + SAMPLE_FREQUENCY_OFFSET);
  uint32_t length = reader.readUint32(sampleOffset + SAMPLE_LENGTH_OFFSET);
  size_t pcmStart = sampleOffset + SAMPLE_HEADER_SIZE;

  if (frequency == 0) {
    frequency = gameData::audio::DEFAULT_SAMPLE_RATE;
  }
  if (pcmStart + length > data.size()) {
    length = static_cast<uint32_t>(data.size() - pcmStart);
  }
  if (length == 0) {
    return false;
  }

  auto wav = pcmToWav(reinterpret_cast<const int8_t *>(data.data() + pcmStart),
                      length, frequency);
  results.push_back({fileId + "_sam" + std::to_string(sampleNumber) + "_" +
                         std::to_string(frequency) + "Hz.wav",
                     std::move(wav)});
  return true;
}

} // namespace

std::vector<ExtractedAudio>
extractStandaloneSamBank(const std::vector<uint8_t> &data,
                         const std::string &fileId) {
  std::vector<ExtractedAudio> results;

  if (data.size() < SAM_BANK_TABLE_OFFSET + SAMPLE_POINTER_SIZE) {
    return results;
  }

  binary::BigEndianReader reader(data);
  uint16_t maxSample = reader.readUint16(0);
  if (maxSample == 0 || maxSample > MAX_STANDALONE_SAMPLE_COUNT) {
    return results;
  }

  size_t tableSize = SAM_BANK_TABLE_OFFSET +
                     static_cast<size_t>(maxSample) * SAMPLE_POINTER_SIZE;
  if (data.size() < tableSize) {
    return results;
  }

  for (uint16_t i = 0; i < maxSample; i++) {
    uint32_t sampleOffset =
        reader.readUint32(SAM_BANK_TABLE_OFFSET + i * SAMPLE_POINTER_SIZE);
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

  if (data.size() < SPRITE_BANK_HEADER_SIZE) {
    return results;
  }

  binary::BigEndianReader reader(data);
  const size_t samBankOffset = reader.readUint32(SAM_BANK_POINTER_OFFSET);
  if (samBankOffset == 0 ||
      samBankOffset + SAM_BANK_TABLE_OFFSET + SAMPLE_POINTER_SIZE >=
          data.size()) {
    return results;
  }

  uint16_t count = reader.readUint16(samBankOffset);
  if (count == 0 || count > MAX_EMBEDDED_SAMPLE_COUNT) {
    return results;
  }

  size_t tableEnd = samBankOffset + SAM_BANK_TABLE_OFFSET +
                    static_cast<size_t>(count) * SAMPLE_POINTER_SIZE;
  if (tableEnd >= data.size()) {
    return results;
  }

  uint32_t previous = 0;
  for (uint16_t i = 0; i < count; i++) {
    const uint32_t offset = reader.readUint32(
        samBankOffset + SAM_BANK_TABLE_OFFSET + i * SAMPLE_POINTER_SIZE);
    if (offset <= previous || samBankOffset + offset >= data.size()) {
      return results;
    }
    previous = offset;
  }

  for (uint16_t i = 0; i < count; i++) {
    size_t sampleOffset =
        samBankOffset +
        reader.readUint32(samBankOffset + SAM_BANK_TABLE_OFFSET +
                          i * SAMPLE_POINTER_SIZE);

    appendSample(data, sampleOffset, i + 1, fileId, results);
  }

  return results;
}

ExtractedAudio wrapMusicBank(const std::vector<uint8_t> &data,
                             const std::string &fileId) {
  std::vector<uint8_t> abk;
  abk.reserve(ABK_HEADER_SIZE + data.size());

  abk.push_back('A');
  abk.push_back('m');
  abk.push_back('B');
  abk.push_back('k');
  binary::pushBigEndian16(abk, ABK_MUSIC_BANK_NUMBER);
  binary::pushBigEndian16(abk, 0);
  binary::pushBigEndian32(
      abk, static_cast<uint32_t>(data.size() + ABK_BANK_NAME_SIZE) |
               ABK_CHIP_MEMORY_FLAG);

  const char *bankName = "Music   ";
  abk.insert(abk.end(), bankName, bankName + ABK_BANK_NAME_SIZE);
  abk.insert(abk.end(), data.begin(), data.end());

  return {fileId + ".abk", std::move(abk)};
}

} // namespace openfranko::lib::converter::audioExtractor
