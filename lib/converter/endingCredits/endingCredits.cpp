#include "endingCredits.h"

#include "../../helpers/helpers.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <stdexcept>

namespace openfranko::lib::converter::endingCredits {
namespace {

constexpr uint32_t HUNK_TYPE_MASK = 0x3FFFFFFF;
constexpr uint32_t HUNK_HEADER = 0x3F3;
constexpr uint32_t HUNK_CODE = 0x3E9;
constexpr uint32_t HUNK_DATA = 0x3EA;
constexpr uint32_t HUNK_BSS = 0x3EB;
constexpr uint32_t HUNK_RELOC32 = 0x3EC;
constexpr uint32_t HUNK_END = 0x3F2;
constexpr uint32_t HUNK_SYMBOL = 0x3F0;
constexpr uint32_t HUNK_DEBUG = 0x3F1;
constexpr uint32_t HUNK_NAME = 0x3E8;

constexpr uint8_t MOVEQ_D3 = 0x76;
constexpr uint16_t PUSH_D3 = 0x2703;
constexpr uint16_t MOVE_LONG_D3 = 0x263C;
constexpr uint16_t PUSH_LONG = 0x273C;
constexpr uint16_t CLEAR_D5 = 0x7A00;
constexpr uint16_t JSR_LONG = 0x4EB9;
constexpr uint16_t JSR_A4 = 0x4EAC;
constexpr uint16_t JMP_A4 = 0x4EEC;
constexpr uint16_t ADDQ_LONG_D3 = 0x5083;
constexpr uint16_t ADDQ_DATA_MASK = 0x0E00;
constexpr int ADDQ_DATA_SHIFT = 9;
constexpr int ADDQ_EIGHT = 8;

constexpr int CREDITS_GLYPH_OFFSET = 6;
constexpr std::size_t CALL_SIZE = 8;

constexpr char FIRST_PRINTABLE = 0x20;
constexpr char LAST_PRINTABLE = 0x7E;

struct Push {
  bool longImmediate = false;
  int32_t value = 0;
  std::size_t start = 0;
};

struct Call {
  std::size_t at = 0;
  uint32_t target = 0;
  std::optional<std::string> text;
  int32_t value = 0;
};

uint16_t word(const std::vector<uint8_t> &code, std::size_t at) {
  return static_cast<uint16_t>(code[at] << 8 | code[at + 1]);
}

int32_t longAt(const std::vector<uint8_t> &code, std::size_t at) {
  return static_cast<int32_t>(static_cast<uint32_t>(word(code, at)) << 16 |
                              word(code, at + 2));
}

std::optional<Push> pushBefore(const std::vector<uint8_t> &code,
                               std::size_t end) {
  if (end >= 4 && word(code, end - 2) == PUSH_D3 && code[end - 4] == MOVEQ_D3) {
    return Push{false, static_cast<int8_t>(code[end - 3]), end - 4};
  }
  if (end >= 8 && word(code, end - 2) == PUSH_D3 &&
      word(code, end - 8) == MOVE_LONG_D3) {
    return Push{false, longAt(code, end - 6), end - 8};
  }
  if (end >= 6 && word(code, end - 6) == PUSH_LONG) {
    return Push{true, longAt(code, end - 4), end - 6};
  }
  return std::nullopt;
}

std::optional<std::string> stringAt(const std::vector<uint8_t> &code,
                                    int32_t offset) {
  if (offset < 0 || offset % 2 != 0 ||
      static_cast<std::size_t>(offset) + 2 > code.size()) {
    return std::nullopt;
  }
  const std::size_t start = static_cast<std::size_t>(offset);
  const std::size_t length = word(code, start);
  if (length == 0 || start + 2 + length > code.size()) {
    return std::nullopt;
  }
  std::string text(code.begin() + static_cast<std::ptrdiff_t>(start + 2),
                   code.begin() +
                       static_cast<std::ptrdiff_t>(start + 2 + length));
  const bool printable = std::all_of(text.begin(), text.end(), [](char c) {
    return c >= FIRST_PRINTABLE && c <= LAST_PRINTABLE;
  });
  return printable ? std::optional<std::string>(text) : std::nullopt;
}

std::vector<Call> procedureCalls(const std::vector<uint8_t> &code) {
  std::vector<Call> calls;
  for (std::size_t at = 0; at + 6 <= code.size(); at += 2) {
    if (word(code, at) != CLEAR_D5 || word(code, at + 2) != JSR_LONG) {
      continue;
    }
    const std::optional<Push> last = pushBefore(code, at);
    if (!last) {
      continue;
    }
    Call call{at, static_cast<uint32_t>(longAt(code, at + 4)), std::nullopt,
              last->value};
    const std::optional<Push> before = pushBefore(code, last->start);
    if (before && before->longImmediate) {
      call.text = stringAt(code, before->value);
    }
    calls.push_back(std::move(call));
  }
  return calls;
}

uint32_t mostFrequent(const std::map<uint32_t, int> &counts) {
  return std::max_element(
             counts.begin(), counts.end(),
             [](const auto &a, const auto &b) { return a.second < b.second; })
      ->first;
}

int glyphOffset(const std::vector<uint8_t> &code, uint32_t font) {
  for (std::size_t at = font; at + 8 <= code.size() && word(code, at) != JMP_A4;
       at += 2) {
    const uint16_t addq = word(code, at + 4);
    if (word(code, at) == JSR_A4 && (addq & ~ADDQ_DATA_MASK) == ADDQ_LONG_D3 &&
        word(code, at + 6) == JSR_A4) {
      const int data = (addq & ADDQ_DATA_MASK) >> ADDQ_DATA_SHIFT;
      return data == 0 ? ADDQ_EIGHT : data;
    }
  }
  return CREDITS_GLYPH_OFFSET;
}

std::string shifted(std::string text, int shift) {
  for (char &c : text) {
    c = static_cast<char>(c + shift);
  }
  return text;
}

std::optional<uint32_t> fontProcedure(const std::vector<Call> &calls) {
  std::map<uint32_t, int> textCalls;
  for (const Call &call : calls) {
    if (call.text) {
      ++textCalls[call.target];
    }
  }
  if (textCalls.empty()) {
    return std::nullopt;
  }
  return mostFrequent(textCalls);
}

bool closesWithBareCall(const std::vector<uint8_t> &code, const Call &call,
                        uint32_t font) {
  const std::size_t next = call.at + CALL_SIZE;
  return next + 6 <= code.size() && word(code, next) == JSR_LONG &&
         static_cast<uint32_t>(longAt(code, next + 2)) != font;
}

std::vector<Page> introIn(const std::vector<uint8_t> &code) {
  const std::vector<Call> calls = procedureCalls(code);
  const std::optional<uint32_t> font = fontProcedure(calls);
  if (!font) {
    return {};
  }
  const int shift = glyphOffset(code, *font) - CREDITS_GLYPH_OFFSET;

  std::vector<Page> pages;
  Page page;
  for (const Call &call : calls) {
    if (call.target != *font || !call.text) {
      page = Page{};
      continue;
    }
    page.lines.push_back({shifted(*call.text, shift), call.value});
    if (closesWithBareCall(code, call, *font)) {
      pages.push_back(std::move(page));
      page = Page{};
    }
  }
  return pages;
}

std::vector<Page> creditsIn(const std::vector<uint8_t> &code) {
  const std::vector<Call> calls = procedureCalls(code);
  const std::optional<uint32_t> found = fontProcedure(calls);
  if (!found) {
    return {};
  }
  const uint32_t font = *found;
  const auto isLine = [font](const Call &call) {
    return call.target == font && call.text;
  };

  std::map<uint32_t, int> callsAfterLines;
  for (std::size_t i = 1; i < calls.size(); ++i) {
    if (isLine(calls[i - 1]) && calls[i].target != font && !calls[i].text) {
      ++callsAfterLines[calls[i].target];
    }
  }
  if (callsAfterLines.empty()) {
    return {};
  }
  const uint32_t beat = mostFrequent(callsAfterLines);
  const int shift = glyphOffset(code, font) - CREDITS_GLYPH_OFFSET;

  std::vector<Page> pages;
  Page page;
  for (const Call &call : calls) {
    if (isLine(call)) {
      page.lines.push_back({shifted(*call.text, shift), call.value});
    } else if (call.target == beat && !page.lines.empty()) {
      page.beat = call.value;
      pages.push_back(std::move(page));
      page = Page{};
    } else {
      page = Page{};
    }
  }
  return pages;
}

std::string escapeJson(const std::string &text) {
  std::string out;
  for (const char c : text) {
    if (c == '"' || c == '\\') {
      out += '\\';
    }
    out += c;
  }
  return out;
}

} // namespace

std::vector<std::vector<uint8_t>>
readHunks(const std::vector<uint8_t> &executable) {
  const helpers::BigEndianReader reader(executable);
  std::size_t at = 0;
  const auto next = [&reader, &at] {
    const uint32_t value = reader.readUint32(at);
    at += 4;
    return value;
  };
  const auto skipLongs = [&at](uint32_t count) {
    at += 4 * static_cast<std::size_t>(count);
  };
  if (executable.size() < 4 || next() != HUNK_HEADER) {
    throw std::runtime_error("Not an AmigaDOS executable");
  }
  for (uint32_t words = next(); words != 0; words = next()) {
    skipLongs(words);
  }
  next();
  const uint32_t first = next();
  const uint32_t last = next();
  skipLongs(last - first + 1);

  std::vector<std::vector<uint8_t>> hunks;
  while (at + 4 <= executable.size()) {
    const uint32_t type = next() & HUNK_TYPE_MASK;
    if (type == HUNK_CODE || type == HUNK_DATA) {
      const std::size_t size = 4 * static_cast<std::size_t>(next());
      if (at + size > executable.size()) {
        throw std::runtime_error("Truncated hunk");
      }
      hunks.emplace_back(executable.begin() + static_cast<std::ptrdiff_t>(at),
                         executable.begin() +
                             static_cast<std::ptrdiff_t>(at + size));
      at += size;
    } else if (type == HUNK_BSS) {
      next();
      hunks.emplace_back();
    } else if (type == HUNK_RELOC32) {
      for (uint32_t count = next(); count != 0; count = next()) {
        skipLongs(count + 1);
      }
    } else if (type == HUNK_SYMBOL) {
      for (uint32_t words = next(); words != 0; words = next()) {
        skipLongs(words + 1);
      }
    } else if (type == HUNK_DEBUG || type == HUNK_NAME) {
      skipLongs(next());
    } else if (type != HUNK_END) {
      throw std::runtime_error("Unsupported hunk type");
    }
  }
  return hunks;
}

std::vector<Page> extract(const std::vector<uint8_t> &executable) {
  for (const std::vector<uint8_t> &hunk : readHunks(executable)) {
    std::vector<Page> pages = creditsIn(hunk);
    if (!pages.empty()) {
      return pages;
    }
  }
  throw std::runtime_error("No ending credits found in the executable");
}

std::vector<Page> extractIntro(const std::vector<uint8_t> &executable) {
  for (const std::vector<uint8_t> &hunk : readHunks(executable)) {
    std::vector<Page> pages = introIn(hunk);
    if (!pages.empty()) {
      return pages;
    }
  }
  return {};
}

std::vector<uint8_t> toJson(const std::vector<Page> &pages) {
  std::string out = "{\n  \"pages\": [\n";
  for (std::size_t p = 0; p < pages.size(); ++p) {
    const Page &page = pages[p];
    out += "    {\n      \"beat\": " + std::to_string(page.beat) +
           ",\n      \"lines\": [\n";
    for (std::size_t l = 0; l < page.lines.size(); ++l) {
      const Line &line = page.lines[l];
      out += "        {\"y\": " + std::to_string(line.y) + ", \"text\": \"" +
             escapeJson(line.text) + "\"}";
      out += l + 1 < page.lines.size() ? ",\n" : "\n";
    }
    out += "      ]\n    }";
    out += p + 1 < pages.size() ? ",\n" : "\n";
  }
  out += "  ]\n}\n";
  return std::vector<uint8_t>(out.begin(), out.end());
}

} // namespace openfranko::lib::converter::endingCredits
