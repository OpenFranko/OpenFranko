#include "json.h"

#include <cstdio>

namespace openfranko::lib::json {
namespace {

constexpr unsigned char FIRST_PRINTABLE = 0x20;

} // namespace

std::string escape(const std::string &text) {
  std::string out;
  for (const char c : text) {
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (static_cast<unsigned char>(c) < FIRST_PRINTABLE) {
      char buf[7];
      snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
      out += buf;
    } else {
      out += c;
    }
  }
  return out;
}

} // namespace openfranko::lib::json
