#include "../../../lib/binary/binary.h"

#include <catch2/catch_all.hpp>

#include <stdexcept>
#include <vector>

using namespace openfranko::lib::binary;

SCENARIO("BigEndianReader reads 32-bit values in big-endian order") {
  GIVEN("A buffer with known bytes") {
    std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02};
    BigEndianReader reader(data);

    WHEN("It is read at offset 0") {
      auto val = reader.readUint32(0);
      THEN("It returns the correct value") { REQUIRE(val == 0xDEADBEEF); }
    }

    WHEN("It is read at offset 2") {
      auto val = reader.readUint32(2);
      THEN("It returns the correct value") { REQUIRE(val == 0xBEEF0102); }
    }
  }

  GIVEN("A buffer of all zeros") {
    std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00};
    BigEndianReader reader(data);

    WHEN("It is read at offset 0") {
      THEN("It returns zero") { REQUIRE(reader.readUint32(0) == 0); }
    }
  }

  GIVEN("A buffer with max values") {
    std::vector<uint8_t> data = {0xFF, 0xFF, 0xFF, 0xFF};
    BigEndianReader reader(data);

    WHEN("It is read at offset 0") {
      THEN("It returns UINT32_MAX") {
        REQUIRE(reader.readUint32(0) == 0xFFFFFFFF);
      }
    }
  }
}

SCENARIO("BigEndianReader reads 16-bit values in big-endian order") {
  GIVEN("A buffer with known bytes") {
    std::vector<uint8_t> data = {0xCA, 0xFE, 0xBA, 0xBE};
    BigEndianReader reader(data);

    WHEN("It is read at offset 0") {
      THEN("It returns 0xCAFE") { REQUIRE(reader.readUint16(0) == 0xCAFE); }
    }

    WHEN("It is read at offset 2") {
      THEN("It returns 0xBABE") { REQUIRE(reader.readUint16(2) == 0xBABE); }
    }
  }
}

SCENARIO("BigEndianReader reads signed 16-bit values") {
  GIVEN("A buffer with a positive value") {
    std::vector<uint8_t> data = {0x00, 0x7F};
    BigEndianReader reader(data);
    THEN("It returns 127") { REQUIRE(reader.readInt16(0) == 127); }
  }

  GIVEN("A buffer with a negative value (0xFFFF = -1)") {
    std::vector<uint8_t> data = {0xFF, 0xFF};
    BigEndianReader reader(data);
    THEN("It returns -1") { REQUIRE(reader.readInt16(0) == -1); }
  }

  GIVEN("A buffer with 0x8000 = -32768") {
    std::vector<uint8_t> data = {0x80, 0x00};
    BigEndianReader reader(data);
    THEN("It returns INT16_MIN") { REQUIRE(reader.readInt16(0) == -32768); }
  }

  GIVEN("A buffer with zero") {
    std::vector<uint8_t> data = {0x00, 0x00};
    BigEndianReader reader(data);
    THEN("It returns 0") { REQUIRE(reader.readInt16(0) == 0); }
  }
}

SCENARIO("BigEndianReader reads signed 32-bit values") {
  GIVEN("A buffer with a positive value") {
    std::vector<uint8_t> data = {0x00, 0x01, 0x00, 0x00};
    BigEndianReader reader(data);
    THEN("It returns 65536") { REQUIRE(reader.readInt32(0) == 65536); }
  }

  GIVEN("A buffer with a negative value (0xFFFFFFFE = -2)") {
    std::vector<uint8_t> data = {0xFF, 0xFF, 0xFF, 0xFE};
    BigEndianReader reader(data);
    THEN("It returns -2") { REQUIRE(reader.readInt32(0) == -2); }
  }
}

SCENARIO("writeLittleEndian16 and writeLittleEndian32 overwrite bytes in "
         "place") {
  GIVEN("A buffer of six zero bytes") {
    std::vector<uint8_t> buf(6, 0);

    WHEN("A 16-bit value is written at offset 1") {
      writeLittleEndian16(buf, 1, 0xCAFE);
      THEN("Its low byte comes first and the rest stays untouched") {
        REQUIRE(buf ==
                std::vector<uint8_t>{0x00, 0xFE, 0xCA, 0x00, 0x00, 0x00});
      }
    }

    WHEN("A 32-bit value is written at offset 2") {
      writeLittleEndian32(buf, 2, 0xDEADBEEF);
      THEN("Its bytes come lowest first") {
        REQUIRE(buf ==
                std::vector<uint8_t>{0x00, 0x00, 0xEF, 0xBE, 0xAD, 0xDE});
      }
    }

    WHEN("A value would run past the end") {
      THEN("The write throws and leaves the buffer as it was") {
        REQUIRE_THROWS_AS(writeLittleEndian16(buf, 5, 1), std::runtime_error);
        REQUIRE_THROWS_AS(writeLittleEndian32(buf, 3, 1), std::runtime_error);
        REQUIRE(buf == std::vector<uint8_t>(6, 0));
      }
    }
  }
}

SCENARIO("BigEndianReader and LittleEndianReader reject truncated reads") {
  GIVEN("A short buffer") {
    std::vector<uint8_t> data = {0x12, 0x34, 0x56};
    BigEndianReader big(data);
    LittleEndianReader little(data);

    THEN("Big-endian reads throw when there are not enough bytes") {
      REQUIRE_THROWS_AS(big.readUint32(0), std::runtime_error);
      REQUIRE_THROWS_AS(big.readUint16(2), std::runtime_error);
    }

    THEN("Little-endian reads throw when there are not enough bytes") {
      REQUIRE_THROWS_AS(little.readUint32(0), std::runtime_error);
      REQUIRE_THROWS_AS(little.readUint16(2), std::runtime_error);
    }
  }
}
