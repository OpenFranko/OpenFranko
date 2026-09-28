#include "IntroStrip.h"

#include "../../assets/Assets.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace openfranko::src::engine::states::presents {
namespace {

constexpr auto INTRO_FILE = "intro.json";
constexpr auto FONT_BOBS = "s50";

constexpr int FRAME_WIDTH = 640;
constexpr int FRAME_HEIGHT = 256;
constexpr int FRAME_DISPLAY_LINE = 42;
constexpr int STRIP_WIDTH = 320;
constexpr int STRIP_HEIGHT = 48;
constexpr int STRIP_DISPLAY_LINE = 136;
constexpr int STRIP_LEFT = 160;
constexpr effects::AmigaColor BLACK = 0x000;

constexpr int LINE_WIDTH = 280;
constexpr int GLYPH_WIDTH = 16;
constexpr int GLYPH_OFFSET = 6;

std::vector<street::CreditPage> loadPages(const std::string &directory) {
  std::ifstream file(std::filesystem::path(directory) / INTRO_FILE);
  if (!file) {
    return {};
  }
  std::stringstream text;
  text << file.rdbuf();
  return street::EndingCredits::fromJson(text.str()).pages;
}

} // namespace

IntroStrip::IntroStrip(systems::VideoSystem &videoSystem,
                       const std::string &directory)
    : m_videoSystem(videoSystem), m_directory(directory),
      m_pages(loadPages(directory)),
      m_rows(effects::visibleRows(FRAME_DISPLAY_LINE, FRAME_HEIGHT,
                                  videoSystem.isNtsc())),
      m_frame(FRAME_WIDTH, m_rows.count) {
  m_strip.width = STRIP_WIDTH;
  m_strip.height = STRIP_HEIGHT;
  m_strip.pixels.assign(static_cast<std::size_t>(STRIP_WIDTH) * STRIP_HEIGHT,
                        0);
}

int IntroStrip::pages() const { return static_cast<int>(m_pages.size()); }

void IntroStrip::show(const effects::BlyskSequence &sequence) {
  if (sequence.page() != m_pasted) {
    std::fill(m_strip.pixels.begin(), m_strip.pixels.end(), 0);
    m_pasted = sequence.page();
    if (m_pasted) {
      paste(*m_pasted);
    }
  }
  m_frame.fill(BLACK);
  m_frame.setPalette(sequence.palette());
  m_frame.draw(m_strip, STRIP_LEFT,
               STRIP_DISPLAY_LINE - FRAME_DISPLAY_LINE - m_rows.first);
  systems::Display display = m_frame.output();
  display.displayHeight = 2 * display.height;
  m_videoSystem.show(display);
}

void IntroStrip::showBlack() {
  m_frame.fill(BLACK);
  systems::Display display = m_frame.output();
  display.displayHeight = 2 * display.height;
  m_videoSystem.show(display);
}

void IntroStrip::paste(int page) {
  if (page < 0 || page >= pages()) {
    return;
  }
  for (const street::CreditLine &line :
       m_pages[static_cast<std::size_t>(page)].lines) {
    const int length = static_cast<int>(line.text.size());
    const int x = (LINE_WIDTH - length * GLYPH_WIDTH) / 2;
    for (int i = 1; i <= length; ++i) {
      const int image = static_cast<unsigned char>(
                            line.text[static_cast<std::size_t>(i - 1)]) +
                        GLYPH_OFFSET;
      const systems::IndexedBitmap *bitmap = glyph(image);
      if (bitmap == nullptr) {
        continue;
      }
      const int left = i * GLYPH_WIDTH + x - bitmap->hotspotX;
      const int top = line.y - bitmap->hotspotY;
      for (int row = 0; row < bitmap->height; ++row) {
        for (int column = 0; column < bitmap->width; ++column) {
          const int stripX = left + column;
          const int stripY = top + row;
          const uint8_t index = bitmap->pixels[static_cast<std::size_t>(
              row * bitmap->width + column)];
          if (index != 0 && stripX >= 0 && stripX < STRIP_WIDTH &&
              stripY >= 0 && stripY < STRIP_HEIGHT) {
            m_strip.pixels[static_cast<std::size_t>(stripY * STRIP_WIDTH +
                                                    stripX)] = index;
          }
        }
      }
    }
  }
}

const systems::IndexedBitmap *IntroStrip::glyph(int image) {
  auto found = m_glyphs.find(image);
  if (found == m_glyphs.end()) {
    std::optional<systems::IndexedBitmap> bitmap;
    const std::string path =
        assets::imagePath(FONT_BOBS, image - 1, m_directory);
    if (std::filesystem::exists(path)) {
      bitmap = systems::loadIndexedBitmap(path);
    }
    found = m_glyphs.emplace(image, std::move(bitmap)).first;
  }
  return found->second ? &*found->second : nullptr;
}

} // namespace openfranko::src::engine::states::presents
