#include "IntroStrip.h"

#include "../../assets/Assets.h"
#include "../../street/core/Font.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>

namespace openfranko::src::engine::states::shared {
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

std::vector<street::core::CreditPage> loadPages(assets::Files &files,
                                                const std::string &directory) {
  const std::string path =
      (std::filesystem::path(directory) / INTRO_FILE).string();
  if (!files.exists(path)) {
    return {};
  }
  const std::vector<uint8_t> text = files.read(path);
  return street::core::EndingCredits::fromJson(
             std::string(text.begin(), text.end()))
      .pages;
}

} // namespace

IntroStrip::IntroStrip(systems::graphics::Monitor &monitor,
                       assets::Files &files, const std::string &directory)
    : m_monitor(monitor), m_files(files), m_directory(directory),
      m_pages(loadPages(files, directory)),
      m_rows(visibleRows(FRAME_DISPLAY_LINE, FRAME_HEIGHT, monitor.isNtsc())),
      m_frame(FRAME_WIDTH, m_rows.count) {
  m_strip.width = STRIP_WIDTH;
  m_strip.height = STRIP_HEIGHT;
  m_strip.pixels.assign(static_cast<std::size_t>(STRIP_WIDTH) * STRIP_HEIGHT,
                        0);
}

int IntroStrip::pages() const { return static_cast<int>(m_pages.size()); }

void IntroStrip::show(const effects::sequences::BlyskSequence &sequence) {
  if (sequence.page() != m_pasted) {
    std::fill(m_strip.pixels.begin(), m_strip.pixels.end(), 0);
    m_pasted = sequence.page();
    if (m_pasted) {
      paste(*m_pasted);
    }
  }
  m_frame.fill(effects::color::BLACK);
  m_frame.setPalette(sequence.palette());
  m_frame.draw(m_strip, STRIP_LEFT,
               STRIP_DISPLAY_LINE - FRAME_DISPLAY_LINE - m_rows.first);
  systems::graphics::Display display = m_frame.output();
  display.displayHeight = 2 * display.height;
  m_monitor.show(display);
}

void IntroStrip::showBlack() {
  m_frame.fill(effects::color::BLACK);
  systems::graphics::Display display = m_frame.output();
  display.displayHeight = 2 * display.height;
  m_monitor.show(display);
}

void IntroStrip::paste(int page) {
  if (page < 0 || page >= pages()) {
    return;
  }
  for (const street::core::CreditLine &line :
       m_pages[static_cast<std::size_t>(page)].lines) {
    street::core::font(line.text, line.y, [this](int x, int y, int image) {
      pasteGlyph(x, y, image);
    });
  }
}

void IntroStrip::pasteGlyph(int x, int y, int image) {
  const systems::graphics::IndexedBitmap *bitmap = glyph(image);
  if (bitmap == nullptr) {
    return;
  }
  const int left = x - bitmap->hotspotX;
  const int top = y - bitmap->hotspotY;
  for (int row = 0; row < bitmap->height; ++row) {
    for (int column = 0; column < bitmap->width; ++column) {
      const int stripX = left + column;
      const int stripY = top + row;
      const uint8_t index =
          bitmap
              ->pixels[static_cast<std::size_t>(row * bitmap->width + column)];
      if (index != 0 && stripX >= 0 && stripX < STRIP_WIDTH && stripY >= 0 &&
          stripY < STRIP_HEIGHT) {
        m_strip
            .pixels[static_cast<std::size_t>(stripY * STRIP_WIDTH + stripX)] =
            index;
      }
    }
  }
}

const systems::graphics::IndexedBitmap *IntroStrip::glyph(int image) {
  auto found = m_glyphs.find(image);
  if (found == m_glyphs.end()) {
    std::optional<systems::graphics::IndexedBitmap> bitmap;
    const std::string path =
        assets::imagePath(FONT_BOBS, image - 1, m_directory);
    if (m_files.exists(path)) {
      bitmap = m_files.loadBitmap(path);
    }
    found = m_glyphs.emplace(image, std::move(bitmap)).first;
  }
  return found->second ? &*found->second : nullptr;
}

} // namespace openfranko::src::engine::states::shared
