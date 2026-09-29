#include "endingCredits.h"
#include "../../binary/binary.h"
#include "../../json/json.h"

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
constexpr size_t CALL_SIZE = 8;

constexpr char FIRST_PRINTABLE = 0x20;
constexpr char LAST_PRINTABLE = 0x7E;

struct Push {
  bool longImmediate = false;
  int32_t value = 0;
  size_t start = 0;
};

struct Call {
  size_t at = 0;
  uint32_t target = 0;
  std::optional<std::string> text;
  int32_t value = 0;
};

std::optional<Push> pushBefore(const std::vector<uint8_t> &code, size_t end) {
  const binary::BigEndianReader reader(code);
  if (end >= 4 && reader.readUint16(end - 2) == PUSH_D3 &&
      code[end - 4] == MOVEQ_D3) {
    return Push{false, static_cast<int8_t>(code[end - 3]), end - 4};
  }
  if (end >= 8 && reader.readUint16(end - 2) == PUSH_D3 &&
      reader.readUint16(end - 8) == MOVE_LONG_D3) {
    return Push{false, reader.readInt32(end - 6), end - 8};
  }
  if (end >= 6 && reader.readUint16(end - 6) == PUSH_LONG) {
    return Push{true, reader.readInt32(end - 4), end - 6};
  }
  return std::nullopt;
}

std::optional<std::string> stringAt(const std::vector<uint8_t> &code,
                                    int32_t offset) {
  if (offset < 0 || offset % 2 != 0 ||
      static_cast<size_t>(offset) + 2 > code.size()) {
    return std::nullopt;
  }
  const size_t start = static_cast<size_t>(offset);
  const size_t length = binary::BigEndianReader(code).readUint16(start);
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
  const binary::BigEndianReader reader(code);
  std::vector<Call> calls;
  for (size_t at = 0; at + CALL_SIZE <= code.size(); at += 2) {
    if (reader.readUint16(at) != CLEAR_D5 ||
        reader.readUint16(at + 2) != JSR_LONG) {
      continue;
    }
    const std::optional<Push> last = pushBefore(code, at);
    if (!last) {
      continue;
    }
    Call call{at, reader.readUint32(at + 4), std::nullopt, last->value};
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
  const binary::BigEndianReader reader(code);
  for (size_t at = font;
       at + 8 <= code.size() && reader.readUint16(at) != JMP_A4; at += 2) {
    const uint16_t addq = reader.readUint16(at + 4);
    if (reader.readUint16(at) == JSR_A4 &&
        (addq & ~ADDQ_DATA_MASK) == ADDQ_LONG_D3 &&
        reader.readUint16(at + 6) == JSR_A4) {
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
      textCalls[call.target]++;
    }
  }
  if (textCalls.empty()) {
    return std::nullopt;
  }
  return mostFrequent(textCalls);
}

bool closesWithBareCall(const std::vector<uint8_t> &code, const Call &call,
                        uint32_t font) {
  const binary::BigEndianReader reader(code);
  const size_t next = call.at + CALL_SIZE;
  return next + 6 <= code.size() && reader.readUint16(next) == JSR_LONG &&
         reader.readUint32(next + 2) != font;
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
  for (size_t i = 1; i < calls.size(); i++) {
    if (isLine(calls[i - 1]) && calls[i].target != font && !calls[i].text) {
      callsAfterLines[calls[i].target]++;
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

} // namespace

std::vector<std::vector<uint8_t>>
readHunks(const std::vector<uint8_t> &executable) {
  const binary::BigEndianReader reader(executable);
  size_t at = 0;
  const auto next = [&reader, &at] {
    const uint32_t value = reader.readUint32(at);
    at += 4;
    return value;
  };
  const auto skipLongs = [&at](uint32_t count) {
    at += 4 * static_cast<size_t>(count);
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
      const size_t size = 4 * static_cast<size_t>(next());
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
  for (size_t i = 0; i < pages.size(); i++) {
    const Page &page = pages[i];
    out += "    {\n      \"beat\": " + std::to_string(page.beat) +
           ",\n      \"lines\": [\n";
    for (size_t j = 0; j < page.lines.size(); j++) {
      const Line &line = page.lines[j];
      out += "        {\"y\": " + std::to_string(line.y) + ", \"text\": \"" +
             json::escape(line.text) + "\"}";
      out += j + 1 < page.lines.size() ? ",\n" : "\n";
    }
    out += "      ]\n    }";
    out += i + 1 < pages.size() ? ",\n" : "\n";
  }
  out += "  ]\n}\n";
  return std::vector<uint8_t>(out.begin(), out.end());
}

} // namespace openfranko::lib::converter::endingCredits
