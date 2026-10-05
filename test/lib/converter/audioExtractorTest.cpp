#include "../../../lib/converter/audioExtractor/audioExtractor.h"

#include "../../../lib/binary/binary.h"

#include <catch2/catch_all.hpp>

#include <string>
#include <vector>

using namespace openfranko::lib::converter::audioExtractor;
using namespace openfranko::lib::binary;

namespace {

std::vector<uint8_t> buildStandaloneSamBank(uint16_t freq, uint32_t pcmLen,
                                            const std::vector<int8_t> &pcm) {
  std::vector<uint8_t> data;
  pushBigEndian16(data, 1);
  uint32_t sampleOffset = 6;
  pushBigEndian32(data, sampleOffset);

  pushBigEndian32(data, 0);
  pushBigEndian32(data, 0);
  pushBigEndian16(data, freq);
  pushBigEndian32(data, pcmLen);
  for (auto s : pcm) {
    data.push_back(static_cast<uint8_t>(s));
  }
  return data;
}

std::vector<uint8_t> sampleRecord(uint16_t frequency, uint32_t length,
                                  const std::vector<int8_t> &pcm) {
  std::vector<uint8_t> record(8, 0);
  pushBigEndian16(record, frequency);
  pushBigEndian32(record, length);
  for (int8_t value : pcm) {
    record.push_back(static_cast<uint8_t>(value));
  }
  return record;
}

std::vector<uint8_t>
standaloneBank(const std::vector<std::vector<uint8_t>> &records) {
  std::vector<uint8_t> data;
  pushBigEndian16(data, static_cast<uint16_t>(records.size()));
  auto offset = static_cast<uint32_t>(2 + records.size() * 4);
  for (const auto &record : records) {
    pushBigEndian32(data, record.empty() ? 0 : offset);
    offset += static_cast<uint32_t>(record.size());
  }
  for (const auto &record : records) {
    data.insert(data.end(), record.begin(), record.end());
  }
  return data;
}

std::vector<uint8_t>
embeddedBank(const std::vector<std::vector<uint8_t>> &records) {
  std::vector<uint8_t> bank;
  pushBigEndian16(bank, static_cast<uint16_t>(records.size()));
  auto offset = static_cast<uint32_t>(2 + records.size() * 4);
  for (const auto &record : records) {
    pushBigEndian32(bank, offset);
    offset += static_cast<uint32_t>(record.size());
  }
  for (const auto &record : records) {
    bank.insert(bank.end(), record.begin(), record.end());
  }
  return bank;
}

std::vector<uint8_t> spriteBankWith(const std::vector<uint8_t> &samBank) {
  const std::vector<uint8_t> sprites(30, 0x5A);
  std::vector<uint8_t> data;
  pushBigEndian16(data, 1);
  pushBigEndian16(data, 16);
  pushBigEndian16(data, 8);
  pushBigEndian16(data, 16);
  pushBigEndian32(data, static_cast<uint32_t>(12 + sprites.size()));
  data.insert(data.end(), sprites.begin(), sprites.end());
  data.insert(data.end(), samBank.begin(), samBank.end());
  return data;
}

std::size_t samBankStart(const std::vector<uint8_t> &data) {
  return BigEndianReader(data).readUint32(8);
}

void setLong(std::vector<uint8_t> &data, std::size_t at, uint32_t value) {
  data.at(at) = static_cast<uint8_t>(value >> 24);
  data.at(at + 1) = static_cast<uint8_t>(value >> 16);
  data.at(at + 2) = static_cast<uint8_t>(value >> 8);
  data.at(at + 3) = static_cast<uint8_t>(value);
}

std::string tag(const std::vector<uint8_t> &wav, std::size_t at) {
  return std::string(wav.begin() + static_cast<std::ptrdiff_t>(at),
                     wav.begin() + static_cast<std::ptrdiff_t>(at + 4));
}

std::vector<uint8_t> wavSamples(const std::vector<uint8_t> &wav) {
  return std::vector<uint8_t>(wav.begin() + 44, wav.end());
}

} // namespace

SCENARIO("extractStandaloneSamBank extracts WAV from sample bank") {
  GIVEN("A sample bank with one sample") {
    std::vector<int8_t> pcm = {0, 10, -10, 50, -50};
    auto data = buildStandaloneSamBank(8000, 5, pcm);

    WHEN("extractStandaloneSamBank is called") {
      auto results = extractStandaloneSamBank(data, "TEST");

      THEN("It extracts one sample") { REQUIRE(results.size() == 1); }

      THEN("The name includes file ID and frequency") {
        REQUIRE(results[0].name == "TEST_sam1_8000Hz.wav");
      }

      THEN("The output is a valid WAV file") {
        auto &wav = results[0].data;
        REQUIRE(wav.size() >= 44);
        REQUIRE(wav[0] == 'R');
        REQUIRE(wav[1] == 'I');
        REQUIRE(wav[2] == 'F');
        REQUIRE(wav[3] == 'F');
        REQUIRE(wav[8] == 'W');
        REQUIRE(wav[9] == 'A');
        REQUIRE(wav[10] == 'V');
        REQUIRE(wav[11] == 'E');
      }

      THEN("WAV sample rate matches input frequency") {
        auto &wav = results[0].data;
        REQUIRE(LittleEndianReader(wav).readUint32(24) == 8000);
      }

      THEN("WAV has correct number of samples") {
        auto &wav = results[0].data;
        uint32_t dataLen = LittleEndianReader(wav).readUint32(40);
        REQUIRE(dataLen == 5);
      }
    }
  }

  GIVEN("A buffer that is too small") {
    std::vector<uint8_t> data = {0x01, 0x02};

    WHEN("extractStandaloneSamBank is called") {
      auto results = extractStandaloneSamBank(data, "X");
      THEN("It returns empty") { REQUIRE(results.empty()); }
    }
  }

  GIVEN("A sample bank with zero samples") {
    std::vector<uint8_t> data;
    pushBigEndian16(data, 0);
    pushBigEndian32(data, 0);

    WHEN("extractStandaloneSamBank is called") {
      auto results = extractStandaloneSamBank(data, "X");
      THEN("It returns empty") { REQUIRE(results.empty()); }
    }
  }

  GIVEN("A sample with zero frequency") {
    std::vector<int8_t> pcm = {42};
    auto data = buildStandaloneSamBank(0, 1, pcm);

    WHEN("extractStandaloneSamBank is called") {
      auto results = extractStandaloneSamBank(data, "F0");
      THEN("Frequency defaults to 8287") {
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].name == "F0_sam1_8287Hz.wav");
        auto &wav = results[0].data;
        REQUIRE(LittleEndianReader(wav).readUint32(24) == 8287);
      }
    }
  }
}

SCENARIO("wrapMusicBank produces a valid ABK wrapper") {
  GIVEN("Minimal music data (12+ bytes with 3 offset pointers)") {
    std::vector<uint8_t> music(20, 0);

    WHEN("wrapMusicBank is called") {
      auto result = wrapMusicBank(music, "025F");

      THEN("The name has .abk extension") {
        REQUIRE(result.name == "025F.abk");
      }

      THEN("Output starts with AmBk magic") {
        REQUIRE(result.data.size() >= 20);
        REQUIRE(result.data[0] == 'A');
        REQUIRE(result.data[1] == 'm');
        REQUIRE(result.data[2] == 'B');
        REQUIRE(result.data[3] == 'k');
      }

      THEN("Music data follows the 20-byte header") {
        REQUIRE(result.data.size() == 20 + music.size());
      }
    }
  }
}

SCENARIO("extractEmbeddedSamBank handles edge cases") {
  GIVEN("A buffer smaller than 12 bytes") {
    std::vector<uint8_t> data = {0, 1, 2, 3};

    WHEN("extractEmbeddedSamBank is called") {
      auto results = extractEmbeddedSamBank(data, "X");
      THEN("It returns empty") { REQUIRE(results.empty()); }
    }
  }

  GIVEN("A buffer with samBankOffset pointing out of range") {
    std::vector<uint8_t> data(20, 0);
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0xFF;
    data[11] = 0xFF;

    WHEN("extractEmbeddedSamBank is called") {
      auto results = extractEmbeddedSamBank(data, "X");
      THEN("It returns empty") { REQUIRE(results.empty()); }
    }
  }

  GIVEN("A buffer with samBankOffset = 0 (no sample bank)") {
    std::vector<uint8_t> data(20, 0);

    WHEN("extractEmbeddedSamBank is called") {
      auto results = extractEmbeddedSamBank(data, "X");
      THEN("It returns empty") { REQUIRE(results.empty()); }
    }
  }
}

SCENARIO("extractStandaloneSamBank skips missing and broken samples") {
  GIVEN("A bank of three samples whose first pointer is empty") {
    const auto data = standaloneBank(
        {{}, sampleRecord(9000, 2, {1, -1}), sampleRecord(10000, 1, {5})});

    WHEN("extractStandaloneSamBank is called") {
      const auto results = extractStandaloneSamBank(data, "S");

      THEN("The other two keep their numbers") {
        REQUIRE(results.size() == 2);
        REQUIRE(results[0].name == "S_sam2_9000Hz.wav");
        REQUIRE(results[1].name == "S_sam3_10000Hz.wav");
        REQUIRE(wavSamples(results[0].data) == std::vector<uint8_t>{129, 127});
      }
    }
  }

  GIVEN("A sample with zero length and a sample longer than the file") {
    const auto data = standaloneBank(
        {sampleRecord(9000, 0, {}), sampleRecord(9000, 100, {7, 8, 9})});

    WHEN("extractStandaloneSamBank is called") {
      const auto results = extractStandaloneSamBank(data, "S");

      THEN("The empty one is skipped and the long one keeps the bytes there "
           "are") {
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].name == "S_sam2_9000Hz.wav");
        REQUIRE(LittleEndianReader(results[0].data).readUint32(40) == 3);
        REQUIRE(wavSamples(results[0].data) ==
                std::vector<uint8_t>{135, 136, 137});
      }
    }
  }

  GIVEN("A sample pointer to the last bytes of the file") {
    auto data = standaloneBank({sampleRecord(9000, 1, {1})});
    setLong(data, 2, static_cast<uint32_t>(data.size() - 4));

    WHEN("extractStandaloneSamBank is called") {
      THEN("There is no room for a sample header, so nothing is extracted") {
        REQUIRE(extractStandaloneSamBank(data, "S").empty());
      }
    }
  }

  GIVEN("A bank whose pointer table runs past the end of the file") {
    std::vector<uint8_t> data;
    pushBigEndian16(data, 3);
    pushBigEndian32(data, 6);

    WHEN("extractStandaloneSamBank is called") {
      THEN("Nothing is extracted") {
        REQUIRE(extractStandaloneSamBank(data, "S").empty());
      }
    }
  }

  GIVEN("A bank that claims more than 100 samples") {
    auto data = standaloneBank({sampleRecord(9000, 1, {1})});
    data[0] = 0;
    data[1] = 101;
    data.resize(2 + 101 * 4 + 20, 0);

    WHEN("extractStandaloneSamBank is called") {
      THEN("Nothing is extracted") {
        REQUIRE(extractStandaloneSamBank(data, "S").empty());
      }
    }
  }
}

SCENARIO("extractEmbeddedSamBank converts the samples stored after a sprite "
         "bank") {
  GIVEN("A sprite bank whose sample bank holds an 11025 Hz sample and one "
        "without a frequency") {
    const auto data =
        spriteBankWith(embeddedBank({sampleRecord(11025, 3, {0, 100, -100}),
                                     sampleRecord(0, 2, {-128, 127})}));

    WHEN("extractEmbeddedSamBank is called") {
      const auto results = extractEmbeddedSamBank(data, "0005");
      REQUIRE(results.size() == 2);

      THEN("Each sample is named after the file, its number and its rate") {
        REQUIRE(results[0].name == "0005_sam1_11025Hz.wav");
        REQUIRE(results[1].name == "0005_sam2_8287Hz.wav");
      }

      THEN("Each WAV is 8-bit mono PCM at the sample's rate") {
        const auto &wav = results[0].data;
        const LittleEndianReader reader(wav);
        REQUIRE(wav.size() == 44 + 3);
        REQUIRE(tag(wav, 0) == "RIFF");
        REQUIRE(reader.readUint32(4) == 36 + 3);
        REQUIRE(tag(wav, 8) == "WAVE");
        REQUIRE(tag(wav, 12) == "fmt ");
        REQUIRE(reader.readUint32(16) == 16);
        REQUIRE(reader.readUint16(20) == 1);
        REQUIRE(reader.readUint16(22) == 1);
        REQUIRE(reader.readUint32(24) == 11025);
        REQUIRE(reader.readUint32(28) == 11025);
        REQUIRE(reader.readUint16(32) == 1);
        REQUIRE(reader.readUint16(34) == 8);
        REQUIRE(tag(wav, 36) == "data");
        REQUIRE(reader.readUint32(40) == 3);
        REQUIRE(LittleEndianReader(results[1].data).readUint32(24) == 8287);
      }

      THEN("The signed samples are stored unsigned") {
        REQUIRE(wavSamples(results[0].data) ==
                std::vector<uint8_t>{128, 228, 28});
        REQUIRE(wavSamples(results[1].data) == std::vector<uint8_t>{0, 255});
      }
    }
  }

  GIVEN("A zero-length first sample, then a sample longer than the file") {
    const auto data = spriteBankWith(embeddedBank(
        {sampleRecord(9000, 0, {}), sampleRecord(9000, 50, {1, 2})}));

    WHEN("extractEmbeddedSamBank is called") {
      const auto results = extractEmbeddedSamBank(data, "X");

      THEN("The empty one is skipped and the long one keeps the bytes there "
           "are") {
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].name == "X_sam2_9000Hz.wav");
        REQUIRE(wavSamples(results[0].data) == std::vector<uint8_t>{129, 130});
      }
    }
  }

  GIVEN("A last sample whose header is cut off by the end of the file") {
    auto records =
        std::vector<std::vector<uint8_t>>{sampleRecord(9000, 1, {1})};
    records.push_back(std::vector<uint8_t>(6, 0));
    const auto data = spriteBankWith(embeddedBank(records));

    WHEN("extractEmbeddedSamBank is called") {
      const auto results = extractEmbeddedSamBank(data, "X");

      THEN("Only the complete sample is extracted") {
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].name == "X_sam1_9000Hz.wav");
      }
    }
  }

  GIVEN("Damaged sample tables") {
    const auto valid = spriteBankWith(
        embeddedBank({sampleRecord(9000, 1, {1}), sampleRecord(9000, 1, {2})}));
    const std::size_t bank = samBankStart(valid);

    THEN("Offsets that do not increase are refused") {
      auto data = valid;
      setLong(data, bank + 6, BigEndianReader(data).readUint32(bank + 2));
      REQUIRE(extractEmbeddedSamBank(data, "X").empty());
    }

    THEN("An offset past the end of the file is refused") {
      auto data = valid;
      setLong(data, bank + 6, static_cast<uint32_t>(data.size()));
      REQUIRE(extractEmbeddedSamBank(data, "X").empty());
    }

    THEN("A bank with no samples or more than 50 is refused") {
      auto data = valid;
      data[bank] = 0;
      data[bank + 1] = 0;
      REQUIRE(extractEmbeddedSamBank(data, "X").empty());
      data[bank + 1] = 51;
      REQUIRE(extractEmbeddedSamBank(data, "X").empty());
    }

    THEN("A table that runs past the end of the file is refused") {
      auto data = valid;
      data[bank + 1] = 50;
      REQUIRE(extractEmbeddedSamBank(data, "X").empty());
    }
  }
}
