#include "fileContainer.h"
#include "../../decompressor/backwardLZ77/backwardLZ77.h"
#include "../../helpers/helpers.h"
#include "../amosCompact/Consts.h"
#include "../gameData/gameData.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <stdexcept>

namespace openfranko::lib::converter::fileContainer {

namespace {

namespace version12 = gameData::version12;
namespace resourceTypes = gameData::resourceTypes;

constexpr size_t SUFFIX_SIZE = 8;

constexpr size_t LONG_PREFIX_SIZE = 4;
constexpr size_t WORD_PREFIX_SIZE = 2;
constexpr size_t LONGWORD_SIZE = 4;
constexpr size_t DATA_HEADER_SIZE = 6;
constexpr size_t DATA16_HEADER_SIZE = 4;

constexpr size_t BOBS_HEADER_SIZE = 10;
constexpr size_t BOBS_COUNT_OFFSET = 4;
constexpr size_t BOBS_SAMPLE_LENGTH_OFFSET = 6;
constexpr size_t BOBS_WIDTH_OFFSET = 8;
constexpr size_t BOBS_HEIGHT_OFFSET = 9;
constexpr size_t BOB_DESCRIPTOR_SIZE = 10;
constexpr size_t BITMAP_PLANES_OFFSET = 14;
constexpr uint16_t FULL_SCREEN_WIDTH = 320;
constexpr size_t VERSION10_BOBS_HEADER_SIZE = 12;

constexpr int CODED_ROTATION = 5;

std::vector<uint8_t> slice(const std::vector<uint8_t> &data, size_t begin,
                           size_t end) {
  if (begin > end || end > data.size()) {
    throw std::runtime_error("File is shorter than its header says");
  }
  return {data.begin() + static_cast<std::ptrdiff_t>(begin),
          data.begin() + static_cast<std::ptrdiff_t>(end)};
}

size_t prefixSize(version12::Loader loader) {
  switch (loader) {
  case version12::Loader::Data:
  case version12::Loader::Music:
  case version12::Loader::Bobs:
    return LONG_PREFIX_SIZE;
  case version12::Loader::Data16:
  case version12::Loader::Stage:
    return WORD_PREFIX_SIZE;
  case version12::Loader::Coded:
    break;
  }
  throw std::runtime_error("File is not squashed");
}

std::vector<uint8_t> unsquash(const std::vector<uint8_t> &rawData,
                              size_t prefix) {
  if (rawData.size() < prefix) {
    throw std::runtime_error("File too small for its packed length");
  }
  helpers::BigEndianReader reader(rawData);
  const size_t packedLength =
      prefix == LONG_PREFIX_SIZE ? reader.readUint32(0) : reader.readUint16(0);
  return decompressor::backwardLZ77::decompressStream(
      slice(rawData, prefix, prefix + packedLength));
}

std::vector<uint8_t> dataBank(const std::vector<uint8_t> &unpacked) {
  const size_t length = helpers::BigEndianReader(unpacked).readUint32(0);
  return slice(unpacked, DATA_HEADER_SIZE, DATA_HEADER_SIZE + length);
}

std::vector<uint8_t> data16Bank(const std::vector<uint8_t> &unpacked) {
  const size_t length = helpers::BigEndianReader(unpacked).readUint16(0);
  if (length < WORD_PREFIX_SIZE) {
    throw std::runtime_error("Data bank is too short");
  }
  std::vector<uint8_t> bank =
      slice(unpacked, DATA16_HEADER_SIZE, WORD_PREFIX_SIZE + length);
  bank.resize(length, 0);
  return bank;
}

std::vector<uint8_t> stageBank(const std::vector<uint8_t> &rawData,
                               const std::vector<uint8_t> &unpacked) {
  std::vector<uint8_t> bank = slice(rawData, 0, WORD_PREFIX_SIZE);
  bank.insert(bank.end(), unpacked.begin(), unpacked.end());
  return bank;
}

std::vector<uint8_t> decodedBank(const std::vector<uint8_t> &rawData) {
  if (rawData.size() % LONGWORD_SIZE != 0) {
    throw std::runtime_error("Coded file is not a whole number of longwords");
  }
  std::vector<uint8_t> bank;
  bank.reserve(rawData.size());
  helpers::BigEndianReader reader(rawData);
  for (size_t pos = 0; pos < rawData.size(); pos += LONGWORD_SIZE) {
    const uint32_t value = reader.readUint32(pos);
    helpers::pushBigEndian32(bank, value << CODED_ROTATION |
                                       value >> (32 - CODED_ROTATION));
  }
  return bank;
}

uint16_t bobColours(const std::vector<uint8_t> &unpacked, size_t count) {
  helpers::BigEndianReader reader(unpacked);
  uint16_t planes = 0;
  for (size_t i = 0; i < count; i++) {
    const size_t picture =
        BOBS_HEADER_SIZE +
        size_t{reader.readUint16(BOBS_HEADER_SIZE + i * BOB_DESCRIPTOR_SIZE)} *
            2;
    if (picture + BITMAP_PLANES_OFFSET + 2 > unpacked.size() ||
        reader.readUint32(picture) != amosCompact::consts::AMOS_BMCODE) {
      continue;
    }
    const uint16_t picturePlanes =
        reader.readUint16(picture + BITMAP_PLANES_OFFSET);
    if (picturePlanes <= amosCompact::consts::MAX_SUPPORTED_BITPLANES) {
      planes = std::max(planes, picturePlanes);
    }
  }
  return planes == 0 ? 0 : static_cast<uint16_t>(1u << planes);
}

std::vector<uint8_t> bobsBank(const std::vector<uint8_t> &unpacked) {
  if (unpacked.size() < BOBS_HEADER_SIZE) {
    throw std::runtime_error("Data too small for a bob bank header");
  }
  helpers::BigEndianReader reader(unpacked);
  const size_t sampleOffset = reader.readUint32(0);
  const size_t count = unpacked[BOBS_COUNT_OFFSET];
  const size_t sampleLength = reader.readUint16(BOBS_SAMPLE_LENGTH_OFFSET);
  const uint16_t width = unpacked[BOBS_WIDTH_OFFSET] == 0
                             ? FULL_SCREEN_WIDTH
                             : unpacked[BOBS_WIDTH_OFFSET];
  const size_t tableEnd = BOBS_HEADER_SIZE + count * BOB_DESCRIPTOR_SIZE;
  if (tableEnd > unpacked.size()) {
    throw std::runtime_error("Data too small for the bob descriptor table");
  }

  const bool offsetInside = sampleOffset >= LONG_PREFIX_SIZE + tableEnd &&
                            sampleOffset - LONG_PREFIX_SIZE <= unpacked.size();
  const size_t picturesEnd =
      offsetInside ? sampleOffset - LONG_PREFIX_SIZE : unpacked.size();
  const std::vector<uint8_t> samples =
      slice(unpacked, picturesEnd, picturesEnd + sampleLength);

  std::vector<uint8_t> bank;
  helpers::pushBigEndian16(bank, static_cast<uint16_t>(count));
  helpers::pushBigEndian16(bank, width);
  helpers::pushBigEndian16(bank, unpacked[BOBS_HEIGHT_OFFSET]);
  helpers::pushBigEndian16(bank, bobColours(unpacked, count));
  helpers::pushBigEndian32(
      bank, samples.empty()
                ? 0
                : static_cast<uint32_t>(VERSION10_BOBS_HEADER_SIZE +
                                        picturesEnd - BOBS_HEADER_SIZE));
  bank.insert(bank.end(),
              unpacked.begin() + static_cast<std::ptrdiff_t>(BOBS_HEADER_SIZE),
              unpacked.begin() + static_cast<std::ptrdiff_t>(picturesEnd));
  bank.insert(bank.end(), samples.begin(), samples.end());
  return bank;
}

Resource unpackVersion12(const version12::File &file,
                         const std::vector<uint8_t> &rawData) {
  Resource resource{std::string(file.name), resourceTypes::ICONS, {}};
  if (file.loader == version12::Loader::Coded) {
    resource.resourceType = resourceTypes::SCREEN_PACKAGE;
    resource.data = decodedBank(rawData);
    return resource;
  }

  const std::vector<uint8_t> unpacked =
      unsquash(rawData, prefixSize(file.loader));
  switch (file.loader) {
  case version12::Loader::Data:
    resource.data = dataBank(unpacked);
    break;
  case version12::Loader::Data16:
    resource.data = data16Bank(unpacked);
    break;
  case version12::Loader::Music:
    resource.resourceType = resourceTypes::MUSIC;
    resource.data = dataBank(unpacked);
    break;
  case version12::Loader::Bobs:
    resource.resourceType = resourceTypes::SPRITES;
    resource.data = bobsBank(unpacked);
    break;
  case version12::Loader::Stage:
    resource.data = stageBank(rawData, unpacked);
    break;
  case version12::Loader::Coded:
    break;
  }
  return resource;
}

Resource unpackVersion10(const std::vector<uint8_t> &rawData) {
  const FileInfo info = parseFooter(rawData);
  Resource resource{fileIdToHex(info.fileId), info.resourceType, {}};
  resource.data = info.compressed
                      ? decompressor::backwardLZ77::decompress(rawData)
                      : slice(rawData, 0, rawData.size() - SUFFIX_SIZE);
  return resource;
}

} // namespace

FileInfo parseFooter(const std::vector<uint8_t> &rawData) {
  if (rawData.size() < SUFFIX_SIZE) {
    throw std::runtime_error("File too small for game suffix");
  }

  size_t off = rawData.size() - SUFFIX_SIZE;
  helpers::BigEndianReader reader(rawData);

  FileInfo info;
  info.unpackSize = reader.readUint32(off);
  info.fileId = reader.readUint16(off + 4);
  info.resourceType = reader.readUint16(off + 6);
  info.bankType = static_cast<uint8_t>(info.resourceType >> 8);
  info.compressed = (info.resourceType & 0xFF) == 0;
  return info;
}

std::string fileIdToHex(uint16_t fileId) {
  char buf[5];
  snprintf(buf, sizeof(buf), "%04X", fileId);
  return buf;
}

Resource unpack(const std::string &fileName,
                const std::vector<uint8_t> &rawData) {
  const version12::File *file = version12::find(fileName);
  return file == nullptr ? unpackVersion10(rawData)
                         : unpackVersion12(*file, rawData);
}

std::vector<uint8_t> unsquashVersion12(const std::string &fileName,
                                       const std::vector<uint8_t> &rawData) {
  const version12::File *file = version12::find(fileName);
  if (file == nullptr) {
    throw std::runtime_error(fileName + " is not a Franko 1.2 data file");
  }
  if (file->loader == version12::Loader::Coded) {
    throw std::runtime_error(fileName + " is not squashed");
  }
  return unsquash(rawData, prefixSize(file->loader));
}

} // namespace openfranko::lib::converter::fileContainer
