#include "codeCards.h"
#include <stdexcept>
#include <string>

namespace openfranko::lib::converter::codeCards {

CodeCards parse(const std::vector<uint8_t> &decompressedData) {
  const size_t end = consts::FIRST_CARD_OFFSET +
                     consts::CARD_COUNT * consts::CARD_SIZE * consts::CARD_SIZE;
  if (decompressedData.size() < end) {
    throw std::runtime_error("Data too small for the code cards");
  }

  CodeCards cards;
  size_t pos = consts::FIRST_CARD_OFFSET;
  for (auto &card : cards) {
    for (auto &row : card.rows) {
      for (auto &cell : row) {
        cell = decompressedData[pos++];
        if (cell >= consts::COLOR_COUNT) {
          throw std::runtime_error("Code card color out of range: " +
                                   std::to_string(cell));
        }
      }
    }
  }
  return cards;
}

std::vector<uint8_t> toJson(const CodeCards &cards) {
  std::string out;
  out += "{\n";
  out += "  \"colors\": {\n";
  for (uint8_t color = 0; color < consts::COLOR_COUNT; ++color) {
    out += "    \"";
    out += static_cast<char>('A' + color);
    out += "\": \"";
    out += colors::name(color);
    out += (color + 1 < consts::COLOR_COUNT) ? "\",\n" : "\"\n";
  }
  out += "  },\n";
  out += "  \"cards\": [\n";

  for (size_t n = 0; n < cards.size(); ++n) {
    out += "    {\n";
    out += "      \"card\": " + std::to_string(n + 1) + ",\n";
    out += "      \"rows\": [\n";
    for (size_t y = 0; y < consts::CARD_SIZE; ++y) {
      out += "        \"";
      for (uint8_t cell : cards[n].rows[y]) {
        out += static_cast<char>('A' + cell);
      }
      out += (y + 1 < consts::CARD_SIZE) ? "\",\n" : "\"\n";
    }
    out += "      ]\n";
    out += (n + 1 < cards.size()) ? "    },\n" : "    }\n";
  }

  out += "  ]\n";
  out += "}\n";
  return std::vector<uint8_t>(out.begin(), out.end());
}

} // namespace openfranko::lib::converter::codeCards
