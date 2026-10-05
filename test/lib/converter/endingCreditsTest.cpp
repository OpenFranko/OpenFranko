#include "../../../lib/converter/endingCredits/endingCredits.h"

#include "../../../lib/binary/binary.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

using namespace openfranko::lib::binary;
using namespace openfranko::lib::converter::endingCredits;

namespace {

constexpr uint32_t FONT = 0x0200;
constexpr uint32_t BEAT = 0x0300;
constexpr uint32_t OTHER = 0x0400;
constexpr uint32_t BLYSK = 0x0500;
constexpr std::size_t STRINGS = 0x0800;
constexpr std::size_t CODE_SIZE = 0x1000;

class Program {
public:
  Program() : m_code(CODE_SIZE, 0) {}

  std::size_t text(const std::string &value) {
    const std::size_t at = m_strings;
    m_code[at] = static_cast<uint8_t>(value.size() >> 8);
    m_code[at + 1] = static_cast<uint8_t>(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
      m_code[at + 2 + i] = static_cast<uint8_t>(value[i]);
    }
    m_strings += 2 + value.size() + value.size() % 2;
    return at;
  }

  void line(const std::string &value, int y) {
    pushLong(static_cast<uint32_t>(text(value)));
    pushInt(y);
    call(FONT);
  }

  void beat(int frames) {
    pushInt(frames);
    call(BEAT);
  }

  void other(int value) {
    pushInt(value);
    call(OTHER);
  }

  void fontAdds(int glyphOffset) {
    const uint16_t addq =
        static_cast<uint16_t>(0x5083 | (glyphOffset & 7) << 9);
    std::size_t at = FONT;
    for (uint16_t value :
         {uint16_t{0x4EAC}, uint16_t{0x1000}, addq, uint16_t{0x4EAC},
          uint16_t{0x1400}, uint16_t{0x4EEC}}) {
      m_code[at++] = static_cast<uint8_t>(value >> 8);
      m_code[at++] = static_cast<uint8_t>(value);
    }
  }

  void blysk() {
    word(0x4EB9);
    word(static_cast<uint16_t>(BLYSK >> 16));
    word(static_cast<uint16_t>(BLYSK));
  }

  void decoy(const std::string &value) {
    pushLong(static_cast<uint32_t>(text(value)));
    pushInt(1);
    call(OTHER);
  }

  void lineWithYThroughD3(const std::string &value, int32_t y) {
    pushLong(static_cast<uint32_t>(text(value)));
    word(0x263C);
    word(static_cast<uint16_t>(static_cast<uint32_t>(y) >> 16));
    word(static_cast<uint16_t>(y));
    word(0x2703);
    call(FONT);
  }

  void lineAt(uint32_t pointer, int y) {
    pushLong(pointer);
    pushInt(y);
    call(FONT);
  }

  void callWithoutArguments(uint32_t target) { call(target); }

  void store(std::size_t at, const std::vector<uint8_t> &bytes) {
    std::copy(bytes.begin(), bytes.end(),
              m_code.begin() + static_cast<std::ptrdiff_t>(at));
  }

  const std::vector<uint8_t> &code() const { return m_code; }

  std::vector<uint8_t> executable() const {
    std::vector<uint8_t> file;
    for (uint32_t value :
         {0x3F3u, 0u, 1u, 0u, 0u, static_cast<uint32_t>(CODE_SIZE / 4), 0x3E9u,
          static_cast<uint32_t>(CODE_SIZE / 4)}) {
      pushBigEndian32(file, value);
    }
    file.insert(file.end(), m_code.begin(), m_code.end());
    pushBigEndian32(file, 0x3F2);
    return file;
  }

private:
  void word(uint16_t value) {
    m_code[m_at++] = static_cast<uint8_t>(value >> 8);
    m_code[m_at++] = static_cast<uint8_t>(value);
  }

  void pushLong(uint32_t value) {
    word(0x273C);
    word(static_cast<uint16_t>(value >> 16));
    word(static_cast<uint16_t>(value));
  }

  void pushInt(int value) {
    if (value >= -128 && value <= 127) {
      word(static_cast<uint16_t>(0x7600 | (value & 0xFF)));
      word(0x2703);
    } else {
      pushLong(static_cast<uint32_t>(value));
    }
  }

  void call(uint32_t target) {
    word(0x7A00);
    word(0x4EB9);
    word(static_cast<uint16_t>(target >> 16));
    word(static_cast<uint16_t>(target));
  }

  std::vector<uint8_t> m_code;
  std::size_t m_at = 0x0600;
  std::size_t m_strings = STRINGS;
};

std::vector<uint8_t> longs(std::initializer_list<uint32_t> values) {
  std::vector<uint8_t> bytes;
  for (uint32_t value : values) {
    pushBigEndian32(bytes, value);
  }
  return bytes;
}

} // namespace

SCENARIO("The ending credits are read from the compiled program's calls") {
  GIVEN("A program that prints two pages between unrelated calls") {
    Program program;
    program.other(-1);
    program.line("AB", 16);
    program.beat(0);
    program.line("C%D", 0);
    program.line("E\\F", 64);
    program.beat(150);
    program.other(20);
    const auto pages = extract(program.executable());

    THEN("Each FONT call is a line and each BLYSK2 call closes a page") {
      REQUIRE(pages.size() == 2);
      REQUIRE(pages[0].lines.size() == 1);
      REQUIRE(pages[0].lines[0].text == "AB");
      REQUIRE(pages[0].lines[0].y == 16);
      REQUIRE(pages[0].beat == 0);
      REQUIRE(pages[1].lines.size() == 2);
      REQUIRE(pages[1].lines[1].text == "E\\F");
      REQUIRE(pages[1].lines[1].y == 64);
      REQUIRE(pages[1].beat == 150);
    }

    THEN("The JSON keeps the stored text, escaped") {
      const auto json = toJson(pages);
      const std::string text(json.begin(), json.end());
      REQUIRE(text.find("\"beat\": 150") != std::string::npos);
      REQUIRE(text.find("{\"y\": 64, \"text\": \"E\\\\F\"}") !=
              std::string::npos);
    }
  }

  GIVEN("A program where another procedure also takes a string, but less "
        "often") {
    Program program;
    program.decoy("ZZ");
    program.line("AA", 0);
    program.line("BB", 8);
    program.beat(10);
    program.decoy("YY");
    program.line("CC", 16);
    program.beat(20);
    const auto pages = extract(program.executable());

    THEN("The procedure called most with a string is taken as FONT") {
      REQUIRE(pages.size() == 2);
      REQUIRE(pages[0].lines.size() == 2);
      REQUIRE(pages[0].lines[0].text == "AA");
      REQUIRE(pages[1].lines.size() == 1);
      REQUIRE(pages[1].lines[0].text == "CC");
    }
  }

  GIVEN("A program whose intro prints lines with no BLYSK2 after them") {
    Program program;
    program.line("XX", 0);
    program.line("YY", 32);
    program.other(5);
    program.line("ZZ", 16);
    program.other(6);
    program.line("AB", 16);
    program.beat(0);
    program.line("CD", 0);
    program.beat(10);
    const auto pages = extract(program.executable());

    THEN("Only the lines closed by BLYSK2 are credits") {
      REQUIRE(pages.size() == 2);
      REQUIRE(pages[0].lines.size() == 1);
      REQUIRE(pages[0].lines[0].text == "AB");
      REQUIRE(pages[1].lines.size() == 1);
      REQUIRE(pages[1].lines[0].text == "CD");
      REQUIRE(pages[1].beat == 10);
    }
  }

  GIVEN("A FONT procedure that adds 8 to the character code, as in 1.2") {
    Program program;
    program.fontAdds(8);
    program.line("AB%", 16);
    program.beat(0);
    const auto pages = extract(program.executable());

    THEN("The text is stored for a FONT that adds 6, as in 1.0") {
      REQUIRE(pages.size() == 1);
      REQUIRE(pages[0].lines[0].text == "CD'");
    }
  }

  GIVEN("A FONT procedure that adds 6 to the character code, as in 1.0") {
    Program program;
    program.fontAdds(6);
    program.line("AB%", 16);
    program.beat(0);
    const auto pages = extract(program.executable());

    THEN("The text is kept as it is") {
      REQUIRE(pages[0].lines[0].text == "AB%");
    }
  }

  GIVEN("Files that are not what the extractor expects") {
    THEN("A non-executable and a program without credits are refused") {
      REQUIRE_THROWS_AS(extract({0, 0, 0, 1}), std::runtime_error);
      Program empty;
      empty.other(3);
      REQUIRE_THROWS_AS(extract(empty.executable()), std::runtime_error);
    }
  }
}

SCENARIO("The 1.2 intro texts are the FONT lines closed by a bare BLYSK") {
  GIVEN("A 1.2 program with two intro pages before its ending credits") {
    Program program;
    program.fontAdds(8);
    program.line("AB", 0);
    program.line("CD", 32);
    program.blysk();
    program.line("EF", 16);
    program.blysk();
    program.other(1);
    program.line("GH", 0);
    program.beat(0);
    program.line("IJ", 8);
    program.beat(10);
    const auto intro = extractIntro(program.executable());

    THEN("Each bare BLYSK call closes a page, in the 1.0 text encoding") {
      REQUIRE(intro.size() == 2);
      REQUIRE(intro[0].lines.size() == 2);
      REQUIRE(intro[0].lines[0].text == "CD");
      REQUIRE(intro[0].lines[0].y == 0);
      REQUIRE(intro[0].lines[1].text == "EF");
      REQUIRE(intro[0].lines[1].y == 32);
      REQUIRE(intro[1].lines.size() == 1);
      REQUIRE(intro[1].lines[0].text == "GH");
      REQUIRE(intro[1].lines[0].y == 16);
    }

    THEN("The ending credits still skip the intro pages") {
      const auto pages = extract(program.executable());
      REQUIRE(pages.size() == 2);
      REQUIRE(pages[0].lines.size() == 1);
      REQUIRE(pages[0].lines[0].text == "IJ");
      REQUIRE(pages[1].lines[0].text == "KL");
      REQUIRE(pages[1].beat == 10);
    }
  }

  GIVEN("A 1.0 program, whose intro has no FONT pages") {
    Program program;
    program.line("AB", 16);
    program.beat(0);

    THEN("No intro pages are found") {
      REQUIRE(extractIntro(program.executable()).empty());
    }
  }
}

SCENARIO("The ending credits reader skips calls it cannot read") {
  GIVEN("Lines whose y is moved into D3 as a long before it is pushed") {
    Program program;
    program.lineWithYThroughD3("AB", 300);
    program.lineWithYThroughD3("CD", -200);
    program.beat(10);
    const auto pages = extract(program.executable());

    THEN("The lines keep their full y values") {
      REQUIRE(pages.size() == 1);
      REQUIRE(pages[0].lines.size() == 2);
      REQUIRE(pages[0].lines[0].text == "AB");
      REQUIRE(pages[0].lines[0].y == 300);
      REQUIRE(pages[0].lines[1].text == "CD");
      REQUIRE(pages[0].lines[1].y == -200);
    }
  }

  GIVEN("A call without pushed arguments between two lines") {
    Program program;
    program.line("AB", 0);
    program.callWithoutArguments(OTHER);
    program.line("CD", 8);
    program.beat(10);
    const auto pages = extract(program.executable());

    THEN("It is ignored and both lines stay on one page") {
      REQUIRE(pages.size() == 1);
      REQUIRE(pages[0].lines.size() == 2);
      REQUIRE(pages[0].lines[0].text == "AB");
      REQUIRE(pages[0].lines[1].text == "CD");
    }
  }

  GIVEN("FONT calls whose text pointer is odd, negative or past the end, or "
        "points at an unprintable, empty or overlong string") {
    Program program;
    program.line("AB", 0);
    program.beat(10);
    program.store(0x0F00, {0x00, 0x02, 'X', 0x07});
    program.store(0x0F10, {0x00, 0x00});
    program.store(0x0FF0, {0x01, 0x00});
    const std::vector<uint32_t> pointers = {static_cast<uint32_t>(STRINGS + 1),
                                            0xFFFFFFF0u,
                                            static_cast<uint32_t>(CODE_SIZE),
                                            0x0F00,
                                            0x0F10,
                                            0x0FF0};
    for (uint32_t pointer : pointers) {
      program.lineAt(pointer, 4);
      program.beat(20);
    }
    program.line("CD", 16);
    program.beat(30);
    const auto pages = extract(program.executable());

    THEN("None of them becomes a line or a page") {
      REQUIRE(pages.size() == 2);
      REQUIRE(pages[0].lines.size() == 1);
      REQUIRE(pages[0].lines[0].text == "AB");
      REQUIRE(pages[1].lines.size() == 1);
      REQUIRE(pages[1].lines[0].text == "CD");
      REQUIRE(pages[1].beat == 30);
    }
  }
}

SCENARIO("Programs without text calls or waits have no credits or intro") {
  GIVEN("A program that calls procedures without any text") {
    Program program;
    program.other(1);
    program.other(2);

    THEN("It has no intro pages") {
      REQUIRE(extractIntro(program.executable()).empty());
    }
  }

  GIVEN("A program that prints lines but never calls anything after them") {
    Program program;
    program.line("AB", 0);
    program.line("CD", 8);

    THEN("No ending credits are found") {
      REQUIRE_THROWS_WITH(extract(program.executable()),
                          "No ending credits found in the executable");
    }
  }
}

SCENARIO("readHunks returns the code, data and BSS hunks of an executable") {
  GIVEN("An executable with a resident library name, relocations, symbols, "
        "debug data, a hunk name and a data hunk for chip memory") {
    const auto file = longs(
        {0x3F3,      1,          0x6C696272, 0,          3,          0,
         2,          2,          1,          4,          0x3E9,      2,
         0x11111111, 0x22222222, 0x3EC,      1,          1,          4,
         0,          0x3F0,      1,          0x73796D31, 0x10,       0,
         0x3F1,      2,          0xAAAAAAAA, 0xBBBBBBBB, 0x3F2,      0x3E8,
         1,          0x64617461, 0x400003EA, 1,          0x33333333, 0x3F2,
         0x3EB,      4,          0x3F2});
    const auto hunks = readHunks(file);

    THEN("The code, data and BSS hunks come back in order") {
      REQUIRE(hunks.size() == 3);
      REQUIRE(hunks[0] == std::vector<uint8_t>{0x11, 0x11, 0x11, 0x11, 0x22,
                                               0x22, 0x22, 0x22});
      REQUIRE(hunks[1] == std::vector<uint8_t>{0x33, 0x33, 0x33, 0x33});
      REQUIRE(hunks[2].empty());
    }
  }

  GIVEN("Credits in a code hunk that follows a BSS hunk") {
    Program program;
    program.line("AB", 16);
    program.beat(10);
    auto file = longs({0x3F3, 0, 2, 0, 1, 4, CODE_SIZE / 4, 0x3EB, 4, 0x3F2,
                       0x3E9, CODE_SIZE / 4});
    file.insert(file.end(), program.code().begin(), program.code().end());
    const auto end = longs({0x3F2});
    file.insert(file.end(), end.begin(), end.end());

    THEN("They are found in the code hunk") {
      const auto pages = extract(file);
      REQUIRE(pages.size() == 1);
      REQUIRE(pages[0].lines[0].text == "AB");
    }
  }

  GIVEN("A code hunk longer than the file") {
    const auto file = longs({0x3F3, 0, 1, 0, 0, 4, 0x3E9, 4, 0, 0});

    THEN("It throws") {
      REQUIRE_THROWS_WITH(readHunks(file), "Truncated hunk");
    }
  }

  GIVEN("A hunk type the reader does not know") {
    const auto file = longs({0x3F3, 0, 1, 0, 0, 0, 0x3F5, 0});

    THEN("It throws") {
      REQUIRE_THROWS_WITH(readHunks(file), "Unsupported hunk type");
    }
  }
}
