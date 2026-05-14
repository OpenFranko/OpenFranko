#include "../../../lib/helpers/helpers.h"
#include <catch2/catch_all.hpp>
#include <stdexcept>
#include <vector>

using namespace openfranko::lib::helpers;

SCENARIO("BigEndianReader reads 32-bit values in big-endian order") {
  GIVEN("A buffer with known bytes") {
    std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02};
    BigEndianReader reader(data);

    WHEN("read at offset 0") {
      auto val = reader.readUint32(0);
      THEN("it returns the correct value") {
        REQUIRE(val == 0xDEADBEEF);
      }
    }

    WHEN("read at offset 2") {
      auto val = reader.readUint32(2);
      THEN("it returns the correct value") {
        REQUIRE(val == 0xBEEF0102);
      }
    }
  }

  GIVEN("A buffer of all zeros") {
    std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00};
    BigEndianReader reader(data);

    WHEN("read at offset 0") {
      THEN("it returns zero") {
        REQUIRE(reader.readUint32(0) == 0);
      }
    }
  }

  GIVEN("A buffer with max values") {
    std::vector<uint8_t> data = {0xFF, 0xFF, 0xFF, 0xFF};
    BigEndianReader reader(data);

    WHEN("read at offset 0") {
      THEN("it returns UINT32_MAX") {
        REQUIRE(reader.readUint32(0) == 0xFFFFFFFF);
      }
    }
  }
}

SCENARIO("BigEndianReader reads 16-bit values in big-endian order") {
  GIVEN("A buffer with known bytes") {
    std::vector<uint8_t> data = {0xCA, 0xFE, 0xBA, 0xBE};
    BigEndianReader reader(data);

    WHEN("read at offset 0") {
      THEN("it returns 0xCAFE") {
        REQUIRE(reader.readUint16(0) == 0xCAFE);
      }
    }

    WHEN("read at offset 2") {
      THEN("it returns 0xBABE") {
        REQUIRE(reader.readUint16(2) == 0xBABE);
      }
    }
  }
}

SCENARIO("BigEndianReader reads signed 16-bit values") {
  GIVEN("A buffer with a positive value") {
    std::vector<uint8_t> data = {0x00, 0x7F};
    BigEndianReader reader(data);
    THEN("it returns 127") {
      REQUIRE(reader.readInt16(0) == 127);
    }
  }

  GIVEN("A buffer with a negative value (0xFFFF = -1)") {
    std::vector<uint8_t> data = {0xFF, 0xFF};
    BigEndianReader reader(data);
    THEN("it returns -1") {
      REQUIRE(reader.readInt16(0) == -1);
    }
  }

  GIVEN("A buffer with 0x8000 = -32768") {
    std::vector<uint8_t> data = {0x80, 0x00};
    BigEndianReader reader(data);
    THEN("it returns INT16_MIN") {
      REQUIRE(reader.readInt16(0) == -32768);
    }
  }

  GIVEN("A buffer with zero") {
    std::vector<uint8_t> data = {0x00, 0x00};
    BigEndianReader reader(data);
    THEN("it returns 0") {
      REQUIRE(reader.readInt16(0) == 0);
    }
  }
}

SCENARIO("binary readers reject truncated reads") {
  GIVEN("A short buffer") {
    std::vector<uint8_t> data = {0x12, 0x34, 0x56};
    BigEndianReader big(data);
    LittleEndianReader little(data);

    THEN("big-endian reads throw when there are not enough bytes") {
      REQUIRE_THROWS_AS(big.readUint32(0), std::runtime_error);
      REQUIRE_THROWS_AS(big.readUint16(2), std::runtime_error);
    }

    THEN("little-endian reads throw when there are not enough bytes") {
      REQUIRE_THROWS_AS(little.readUint32(0), std::runtime_error);
      REQUIRE_THROWS_AS(little.readUint16(2), std::runtime_error);
    }
  }
}
