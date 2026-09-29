#include "AdvertsState.h"

#include "../../assets/Assets.h"

#include <cstddef>
#include <string>

namespace openfranko::src::engine::states::adverts {
namespace {

constexpr int FIRST_SLIDE = 80;
constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 256;
constexpr int DISPLAY_LINE = 42;
constexpr std::size_t SCREEN_COLORS = 32;
constexpr int SCREENS = 2;

effects::color::AmigaPalette
screenPalette(const systems::graphics::IndexedBitmap &picture) {
  effects::color::AmigaPalette palette = picture.palette;
  palette.resize(SCREEN_COLORS);
  return palette;
}

} // namespace

AdvertsState::AdvertsState(systems::graphics::Monitor &monitor,
                           systems::input::ControllerSystem &controllerSystem,
                           assets::Files &files)
    : m_monitor(monitor), m_controllerSystem(controllerSystem),
      m_rows(visibleRows(DISPLAY_LINE, SCREEN_HEIGHT, monitor.isNtsc())),
      m_screen(SCREEN_WIDTH, m_rows.count),
      m_palette(SCREEN_COLORS, effects::color::BLACK) {
  for (int slide = 0; slide < SLIDES; ++slide) {
    m_slides.push_back(files.loadBitmap(
        assets::picturePath("p" + std::to_string(FIRST_SLIDE + slide))));
  }
}

std::optional<EngineStateId> AdvertsState::update() {
  if (m_step == Step::Finished) {
    return EngineStateId::Presents;
  }
  m_fader.advance(m_palette);
  runBasic(m_controllerSystem.states.button);
  show();
  ++m_frame;
  return std::nullopt;
}

void AdvertsState::runBasic(bool fire) {
  while (m_frame >= m_resumeFrame && m_step != Step::Finished) {
    switch (m_step) {
    case Step::Open:
      wait(SCREEN_OPEN_VBLS, Step::Show);
      break;
    case Step::Show:
      m_copied = m_slide;
      m_fader.start(m_palette, FADE_SPEED,
                    screenPalette(m_slides[static_cast<std::size_t>(m_slide)]));
      m_count = 0;
      m_step = Step::FirstPause;
      break;
    case Step::FirstPause:
      if (++m_count <= KLIKER_FRAMES && !fire) {
        wait(1, Step::FirstPause);
      } else if (fire) {
        m_step = Step::Close;
      } else {
        m_fader.start(
            m_palette, FADE_SPEED,
            effects::color::AmigaPalette(SCREEN_COLORS, effects::color::BLACK));
        m_count = 0;
        m_step = Step::SecondPause;
      }
      break;
    case Step::SecondPause:
      if (++m_count <= KLIKER_FRAMES && !fire) {
        wait(1, Step::SecondPause);
      } else if (fire || ++m_slide == SLIDES) {
        m_step = Step::Close;
      } else {
        wait(SCREEN_REOPEN_VBLS, Step::Show);
      }
      break;
    case Step::Close:
      m_closeFrame = m_frame;
      wait(SCREENS * SCREEN_CLOSE_VBLS, Step::Closed);
      break;
    case Step::Closed:
      m_step = Step::Finished;
      break;
    case Step::Finished:
      break;
    }
  }
}

void AdvertsState::wait(int frames, Step next) {
  m_resumeFrame = m_frame + frames;
  m_step = next;
}

void AdvertsState::show() {
  const bool closed =
      m_closeFrame && m_frame >= *m_closeFrame + SCREEN_CLOSE_SHOWN_VBLS;
  if (!m_copied || closed) {
    m_screen.fill(effects::color::BLACK);
  } else {
    m_screen.setPalette(m_palette);
    m_screen.draw(m_slides[static_cast<std::size_t>(*m_copied)], 0,
                  -m_rows.first);
  }
  m_monitor.show(m_screen.output());
}

} // namespace openfranko::src::engine::states::adverts
