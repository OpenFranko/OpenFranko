#include "../../../lib/decompressor/helpers/helpers.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::lib::decompressor::helpers;

SCENARIO("readUint32BigEndian reads 4 bytes in big-endian order") {
  GIVEN("A buffer with known bytes") {
    std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02};

    WHEN("read at offset 0") {
      auto val = readUint32BigEndian(data, 0);
      THEN("it returns the correct value") {
        REQUIRE(val == 0xDEADBEEF);
      }
    }

    WHEN("read at offset 2") {
      auto val = readUint32BigEndian(data, 2);
      THEN("it returns the correct value") {
        REQUIRE(val == 0xBEEF0102);
      }
    }
  }

  GIVEN("A buffer of all zeros") {
    std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00};

    WHEN("read at offset 0") {
      THEN("it returns zero") {
        REQUIRE(readUint32BigEndian(data, 0) == 0);
      }
    }
  }

  GIVEN("A buffer with max values") {
    std::vector<uint8_t> data = {0xFF, 0xFF, 0xFF, 0xFF};

    WHEN("read at offset 0") {
      THEN("it returns UINT32_MAX") {
        REQUIRE(readUint32BigEndian(data, 0) == 0xFFFFFFFF);
      }
    }
  }
}

SCENARIO("readUint16BigEndian reads 2 bytes in big-endian order") {
  GIVEN("A buffer with known bytes") {
    std::vector<uint8_t> data = {0xCA, 0xFE, 0xBA, 0xBE};

    WHEN("read at offset 0") {
      THEN("it returns 0xCAFE") {
        REQUIRE(readUint16BigEndian(data, 0) == 0xCAFE);
      }
    }

    WHEN("read at offset 2") {
      THEN("it returns 0xBABE") {
        REQUIRE(readUint16BigEndian(data, 2) == 0xBABE);
      }
    }
  }
}

SCENARIO("readInt16BigEndian reads signed 16-bit values") {
  GIVEN("A buffer with a positive value") {
    std::vector<uint8_t> data = {0x00, 0x7F};
    THEN("it returns 127") {
      REQUIRE(readInt16BigEndian(data, 0) == 127);
    }
  }

  GIVEN("A buffer with a negative value (0xFFFF = -1)") {
    std::vector<uint8_t> data = {0xFF, 0xFF};
    THEN("it returns -1") {
      REQUIRE(readInt16BigEndian(data, 0) == -1);
    }
  }

  GIVEN("A buffer with 0x8000 = -32768") {
    std::vector<uint8_t> data = {0x80, 0x00};
    THEN("it returns INT16_MIN") {
      REQUIRE(readInt16BigEndian(data, 0) == -32768);
    }
  }

  GIVEN("A buffer with zero") {
    std::vector<uint8_t> data = {0x00, 0x00};
    THEN("it returns 0") {
      REQUIRE(readInt16BigEndian(data, 0) == 0);
    }
  }
}
