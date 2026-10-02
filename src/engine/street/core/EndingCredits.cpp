#include "EndingCredits.h"

#include "Json.h"

#include <utility>

namespace openfranko::src::engine::street::core {
namespace {

CreditLine readLine(JsonCursor &cursor) {
  CreditLine line;
  bool hasText = false;
  bool hasY = false;
  cursor.openObject();
  std::string key;
  while (cursor.nextMember(key)) {
    if (key == "text" && !hasText) {
      line.text = cursor.text();
      hasText = true;
    } else if (key == "y" && !hasY) {
      line.y = cursor.integer();
      hasY = true;
    } else {
      cursor.skip();
    }
  }
  if (!hasText) {
    missingMember("text");
  }
  if (!hasY) {
    missingMember("y");
  }
  return line;
}

CreditPage readPage(JsonCursor &cursor) {
  CreditPage page;
  bool hasBeat = false;
  bool hasLines = false;
  cursor.openObject();
  std::string key;
  while (cursor.nextMember(key)) {
    if (key == "beat" && !hasBeat) {
      page.beat = cursor.integer();
      hasBeat = true;
    } else if (key == "lines" && !hasLines) {
      hasLines = true;
      cursor.openArray();
      while (cursor.nextItem()) {
        page.lines.push_back(readLine(cursor));
      }
    } else {
      cursor.skip();
    }
  }
  if (!hasBeat) {
    missingMember("beat");
  }
  if (!hasLines) {
    missingMember("lines");
  }
  return page;
}

} // namespace

EndingCredits EndingCredits::fromJson(const std::string &json) {
  EndingCredits credits;
  EndingCreditsReader reader(json);
  while (!reader.step(credits)) {
  }
  return credits;
}

EndingCreditsReader::EndingCreditsReader(std::string json)
    : m_json(std::move(json)) {}

bool EndingCreditsReader::step(EndingCredits &credits) {
  if (!m_opened) {
    m_json.openObject();
    m_opened = true;
  }
  if (m_inPages) {
    if (m_json.nextItem()) {
      credits.pages.push_back(readPage(m_json));
      return false;
    }
    m_inPages = false;
  }
  std::string key;
  while (m_json.nextMember(key)) {
    if (key == "pages" && !m_hasPages) {
      m_hasPages = true;
      m_inPages = true;
      m_json.openArray();
      return false;
    }
    m_json.skip();
  }
  m_json.finish();
  if (!m_hasPages) {
    missingMember("pages");
  }
  return true;
}

} // namespace openfranko::src::engine::street::core
