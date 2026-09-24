#include "../../../lib/converter/endingCredits/endingCredits.h"
#include "../../../lib/helpers/helpers.h"
#include <catch2/catch_all.hpp>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using namespace openfranko::lib;
namespace credits = openfranko::lib::converter::endingCredits;

namespace {

constexpr uint32_t FONT = 0x0200;
constexpr uint32_t BEAT = 0x0300;
constexpr uint32_t OTHER = 0x0400;
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

  void decoy(const std::string &value) {
    pushLong(static_cast<uint32_t>(text(value)));
    pushInt(1);
    call(OTHER);
  }

  std::vector<uint8_t> executable() const {
    std::vector<uint8_t> file;
    for (uint32_t value :
         {0x3F3u, 0u, 1u, 0u, 0u, static_cast<uint32_t>(CODE_SIZE / 4), 0x3E9u,
          static_cast<uint32_t>(CODE_SIZE / 4)}) {
      helpers::pushBigEndian32(file, value);
    }
    file.insert(file.end(), m_code.begin(), m_code.end());
    helpers::pushBigEndian32(file, 0x3F2);
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
    const auto pages = credits::extract(program.executable());

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
      const auto json = credits::toJson(pages);
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
    const auto pages = credits::extract(program.executable());

    THEN("The procedure called most with a string is taken as FONT") {
      REQUIRE(pages.size() == 2);
      REQUIRE(pages[0].lines.size() == 2);
      REQUIRE(pages[0].lines[0].text == "AA");
      REQUIRE(pages[1].lines.size() == 1);
      REQUIRE(pages[1].lines[0].text == "CC");
    }
  }

  GIVEN("Files that are not what the extractor expects") {
    THEN("A non-executable and a program without credits are refused") {
      REQUIRE_THROWS_AS(credits::extract({0, 0, 0, 1}), std::runtime_error);
      Program empty;
      empty.other(3);
      REQUIRE_THROWS_AS(credits::extract(empty.executable()),
                        std::runtime_error);
    }
  }
}
