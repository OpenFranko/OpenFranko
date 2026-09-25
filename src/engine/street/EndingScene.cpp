#include "EndingScene.h"

#include "../amal/Actors.h"
#include "../effects/AmigaDisplay.h"
#include "StageFrame.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace openfranko::src::engine::street {
namespace {

constexpr int RO = 14;

constexpr int DANCE_SET = 0x38;
constexpr int STILL_SET = 0x37;
constexpr int STILL_PICTURE = 0x3BD;
constexpr int ENDING_TUNE = 0x25F;
constexpr int FIRST_IMAGE = 1;

constexpr int FULL_VOLUME = 63;
constexpr effects::AmigaColor STAGE_BORDER = 0x555;
constexpr effects::AmigaColor BLACK = 0x000;
constexpr effects::AmigaColor DEFAULT_COLOR = 0x000;
constexpr effects::AmigaColor WHITE = 0xFFF;
constexpr effects::AmigaColor STILL_GREY = 0x444;
constexpr effects::AmigaColor INK = 0xFFF;
constexpr effects::AmigaColor SHADE = 0xAAA;

constexpr int STAGE_WIDTH = 304;

constexpr int AUTOBACK_VBLS = 3;
constexpr int SCREEN_OPEN_VBLS = 1;
constexpr int SCREEN_CLOSE_VBLS = 2;
constexpr int DOUBLE_BUFFER_VBLS = 3;
constexpr int KLIKER_FRAMES = 2000;

constexpr int STILL_TOP = 50;
constexpr int STILL_HEIGHT = 256;
constexpr std::size_t STILL_COLORS = 32;
constexpr int FOTO_OPEN_VBLS = 2;
constexpr int FOTO_WHITE_WAIT = 5;
constexpr int FOTO_SPEED = 4;
constexpr int FOTO_WAIT = 60;
constexpr int STILL_HOLD = 15;
constexpr int STILL_CLEAR_WAIT = 20;
constexpr int STILL_FADE_SPEED = 5;
constexpr int STILL_FADE_WAIT = 50;

constexpr int TEXT_BOX = 1;
constexpr int TEXT_BOX_X = 40;
constexpr int TEXT_BOX_Y = 80;
constexpr int TEXT_BOX_IMAGE = 1;
constexpr int DOORWAY_X = 80;
constexpr int DOORWAY_Y = 128;
constexpr int DOORWAY_IMAGE = 25;
constexpr int FAREWELL_X = 40;
constexpr int FAREWELL_Y = 16;
constexpr int FAREWELL_IMAGE = 2;
constexpr int WALKER = 2;
constexpr int WALKER_X = 142;
constexpr int WALKER_Y = 148;
constexpr int WALKER_IMAGE = 3;

constexpr int DANCER_TOP = 131;
constexpr int DANCER_HEIGHT = 164;
constexpr int DANCER = 1;
constexpr int DANCER_X = 380;
constexpr int DANCER_Y = 163;
constexpr int DANCER_IMAGE = 0x8006;
constexpr int PORTRAITS = 3;
constexpr int PORTRAIT_X = 400;
constexpr int PORTRAIT_Y = 15;

constexpr int TEXT_TOP = 50;
constexpr int TEXT_HEIGHT = 80;
constexpr std::size_t TEXT_COLORS = 16;
constexpr int LINE_WIDTH = 280;
constexpr int GLYPH_WIDTH = 16;
constexpr int GLYPH_OFFSET = 6;
constexpr int BEAT_SPEED = 10;
constexpr int BEAT_HOLD = 150;
constexpr int BEAT_DARK = 150;

const effects::AmigaPalette DANCER_PALETTE = {
    0x000, 0x06F, 0x730, 0x840, 0x950, 0xA60, 0xB70, 0xC80,
    0xD90, 0xEA0, 0xFB0, 0xFC1, 0xFD2, 0xFE3, 0xFF4, 0xFFF};

effects::AmigaPalette beat(effects::AmigaColor ink, effects::AmigaColor shade) {
  effects::AmigaPalette target(TEXT_COLORS, effects::PaletteFader::KEEP);
  target[0] = BLACK;
  target[1] = ink;
  target[2] = shade;
  return target;
}

} // namespace

EndingScene::EndingScene(StreetHost &host, GameSession &session, bool ntsc)
    : m_host(host), m_session(session), m_machine(session.registers),
      m_display(0, 0), m_border(STAGE_BORDER), m_ntsc(ntsc),
      m_displayLine(effects::pictureLine(DISPLAY_LINE, ntsc)) {}

void EndingScene::advance(int16_t joystick) {
  if (m_step == Step::Finished) {
    return;
  }
  if (m_stage) {
    stageFrame();
  }
  if (m_dancerBuffer) {
    m_dancerBuffer->vbl();
  }
  m_machine.tick();
  m_fader.tick(m_screens[0].palette);
  if (m_dancerBuffer && !holdsAtStart()) {
    m_dancerBuffer->test(m_bobs, m_images);
  }
  runBasic(joystick);
  if (m_dancerBuffer && !holdsAtEnd()) {
    m_dancerBuffer->test(m_bobs, m_images);
  }
  redraw();
  ++m_frame;
}

void EndingScene::compose(std::vector<uint32_t> &frame) const {
  frame.assign(static_cast<std::size_t>(WIDTH * HEIGHT), toArgb(m_border));
  if (m_stageShown && m_stage) {
    const IndexedSurface &display = m_stage->buffer.shown();
    const std::size_t mask = m_stage->palette.size() - 1;
    const int rowsPerLine = m_stage->laced ? 2 : 1;
    for (int row = 0; row < HEIGHT; ++row) {
      const int y = (m_displayLine + row - m_stage->displayY) * rowsPerLine;
      if (y < 0 || y >= display.height()) {
        continue;
      }
      for (int x = 0; x < STAGE_WIDTH; ++x) {
        const int column = x + m_stage->offsetX;
        if (column < display.width()) {
          frame[static_cast<std::size_t>(row * WIDTH + x)] =
              toArgb(m_stage->palette[display.pixel(column, y) & mask]);
        }
      }
    }
  }
  if (const IndexedSurface *shown = panel()) {
    const IndexedSurface &surface = *shown;
    const effects::AmigaPalette &colors = panelPalette();
    for (int y = 0; y < StatusPanel::VISIBLE_HEIGHT; ++y) {
      const int row = m_panelTop - m_displayLine + y;
      if (row < 0 || row >= HEIGHT) {
        continue;
      }
      for (int x = 0; x < StatusPanel::WIDTH; ++x) {
        frame[static_cast<std::size_t>(row * WIDTH + x)] =
            toArgb(colors[surface.pixel(x, y)]);
      }
    }
  }
  for (int number : {1, 0}) {
    const Screen &screen = m_screens[static_cast<std::size_t>(number)];
    if (!screen.open || screen.hidden) {
      continue;
    }
    const IndexedSurface &surface =
        number == 1 && m_dancerBuffer
            ? m_dancerBuffer->shown()
            : (number == m_bobScreen ? m_display : screen.surface);
    const std::size_t mask = screen.palette.size() - 1;
    for (int y = 0; y < surface.height(); ++y) {
      const int row = screen.top - m_displayLine + y;
      if (row < 0 || row >= HEIGHT) {
        continue;
      }
      for (int x = 0; x < WIDTH && x < surface.width(); ++x) {
        frame[static_cast<std::size_t>(row * WIDTH + x)] =
            toArgb(screen.palette[surface.pixel(x, y) & mask]);
      }
    }
  }
}

bool EndingScene::isLoading() const {
  return m_step == Step::Start || m_step == Step::Era ||
         m_step == Step::Loading;
}

bool EndingScene::isShowingStill() const {
  return m_step == Step::FotoFade || m_step == Step::FotoClose ||
         m_step == Step::Still || m_step == Step::StillHidden ||
         m_step == Step::StillKliker || m_step == Step::StillOff ||
         m_step == Step::Grey;
}

bool EndingScene::isWalkingAway() const {
  return m_step == Step::Farewell || m_step == Step::WalkAway ||
         m_step == Step::FarewellKliker || m_step == Step::CloseStill;
}

bool EndingScene::isShowingCredits() const {
  return m_step == Step::Page || m_step == Step::PageFade ||
         m_step == Step::PageClear;
}

bool EndingScene::isFinished() const { return m_step == Step::Finished; }

int EndingScene::page() const { return m_page; }

bool EndingScene::isShown(int screen) const {
  const Screen &shown = m_screens[static_cast<std::size_t>(screen)];
  return shown.open && !shown.hidden;
}

bool EndingScene::isStageShown() const { return m_stageShown; }

int EndingScene::displayLine() const { return m_displayLine; }

effects::AmigaColor EndingScene::border() const { return m_border; }

const IndexedSurface &EndingScene::screen(int number) const {
  return m_screens[static_cast<std::size_t>(number)].surface;
}

const effects::AmigaPalette &EndingScene::palette(int number) const {
  return m_screens[static_cast<std::size_t>(number)].palette;
}

const BobLayer &EndingScene::bobs() const { return m_bobs; }

const IndexedSurface *EndingScene::panel() const {
  if (!m_panelShown) {
    return nullptr;
  }
  if (m_panel) {
    return &m_panel->surface();
  }
  return m_stage ? &m_stage->panel : nullptr;
}

amal::Machine &EndingScene::machine() { return m_machine; }

EndingScene::Flow EndingScene::hold(int frames, Step next) {
  m_holdStart = m_frame;
  m_holdUntil = m_frame + frames;
  return wait(frames, next);
}

bool EndingScene::holdsAtStart() const {
  return m_holdStart < m_frame && m_frame <= m_holdUntil;
}

bool EndingScene::holdsAtEnd() const {
  return m_holdStart <= m_frame && m_frame < m_holdUntil;
}

void EndingScene::stageFrame() {
  m_stage->buffer.vbl();
  if (m_stage->buffer.isAutobacking()) {
    m_stage->buffer.autobackStep(m_bobs, m_images);
  }
}

EndingScene::Flow EndingScene::wait(int frames, Step next) {
  m_resumeFrame = m_frame + frames;
  m_step = next;
  return Flow::Yield;
}

bool EndingScene::kliker(int16_t joystick) {
  if (++m_count <= KLIKER_FRAMES && joystick <= 0) {
    return false;
  }
  m_count = 0;
  return true;
}

void EndingScene::runBasic(int16_t joystick) {
  Flow flow = Flow::Continue;
  while (flow == Flow::Continue && m_frame >= m_resumeFrame) {
    switch (m_step) {
    case Step::Start:
      start();
      flow = hold(AUTOBACK_VBLS, Step::Era);
      break;
    case Step::Era:
      era();
      break;
    case Step::Loading:
      if (!m_loading.advance(m_panel.get())) {
        flow = Flow::Yield;
        break;
      }
      m_stageShown = false;
      flow = hold(SCREEN_CLOSE_VBLS, Step::ClosePanel);
      break;
    case Step::ClosePanel:
      m_panelShown = false;
      m_stage.reset();
      flow = hold(SCREEN_CLOSE_VBLS, Step::Foto);
      break;
    case Step::Foto:
      m_host.setMusicVolume(FULL_VOLUME);
      m_host.playMusic();
      flow = hold(FOTO_OPEN_VBLS, Step::FotoWhite);
      break;
    case Step::FotoWhite:
      fotoWhite();
      flow = wait(FOTO_WHITE_WAIT, Step::FotoFade);
      break;
    case Step::FotoFade:
      m_fader.start(m_screens[0].palette, FOTO_SPEED, m_picturePalette);
      flow = wait(FOTO_WAIT, Step::FotoClose);
      break;
    case Step::FotoClose:
      flow = hold(SCREEN_CLOSE_VBLS, Step::Still);
      break;
    case Step::Still:
      m_bobs.set(TEXT_BOX, TEXT_BOX_X, TEXT_BOX_Y, TEXT_BOX_IMAGE);
      flow = hold(SCREEN_OPEN_VBLS, Step::StillHidden);
      break;
    case Step::StillHidden:
      hideStill();
      m_count = 0;
      m_step = Step::StillKliker;
      break;
    case Step::StillKliker:
      flow = kliker(joystick) ? wait(STILL_HOLD, Step::StillOff) : Flow::Yield;
      break;
    case Step::StillOff:
      off();
      flow = wait(STILL_CLEAR_WAIT, Step::Grey);
      break;
    case Step::Grey:
      m_screens[0].surface.fill(0);
      m_fader.start(m_screens[0].palette, STILL_FADE_SPEED,
                    m_screens[1].palette);
      flow = wait(STILL_FADE_WAIT, Step::Farewell);
      break;
    case Step::Farewell:
      farewell();
      m_step = Step::WalkAway;
      break;
    case Step::WalkAway:
      if (m_machine.isRunning(WALKER)) {
        flow = Flow::Yield;
        break;
      }
      m_count = 0;
      m_step = Step::FarewellKliker;
      break;
    case Step::FarewellKliker:
      if (!kliker(joystick)) {
        flow = Flow::Yield;
        break;
      }
      m_screens[0].surface.fill(0);
      m_fader.start(m_screens[0].palette, STILL_FADE_SPEED,
                    effects::AmigaPalette(STILL_COLORS, BLACK));
      flow = wait(STILL_FADE_WAIT, Step::CloseStill);
      break;
    case Step::CloseStill:
      off();
      closeScreen(0);
      flow = hold(SCREEN_CLOSE_VBLS, Step::CloseHidden);
      break;
    case Step::CloseHidden:
      closeScreen(1);
      flow = hold(SCREEN_CLOSE_VBLS, Step::Dancer);
      break;
    case Step::Dancer:
      std::swap(m_images, m_parked);
      flow = hold(SCREEN_OPEN_VBLS, Step::DancerShown);
      break;
    case Step::DancerShown:
      openScreen(1, DANCER_TOP, DANCER_HEIGHT, DANCER_PALETTE);
      m_dancerBuffer.emplace(m_screens[1].surface);
      m_bobScreen = 1;
      flow = hold(DOUBLE_BUFFER_VBLS, Step::Dance);
      break;
    case Step::Dance:
      dance();
      flow = hold(SCREEN_OPEN_VBLS, Step::TextScreen);
      break;
    case Step::TextScreen:
      textScreen();
      m_page = 0;
      m_count = 0;
      m_step = m_credits.pages.empty() ? Step::FinalKliker : Step::Page;
      break;
    case Step::Page:
      pageUp();
      flow = wait(BEAT_HOLD +
                      m_credits.pages[static_cast<std::size_t>(m_page)].beat,
                  Step::PageFade);
      break;
    case Step::PageFade:
      m_fader.start(m_screens[0].palette, BEAT_SPEED, beat(BLACK, BLACK));
      flow = wait(BEAT_DARK, Step::PageClear);
      break;
    case Step::PageClear:
      m_screens[0].surface.fill(0);
      ++m_page;
      m_count = 0;
      m_step = m_page < static_cast<int>(m_credits.pages.size())
                   ? Step::Page
                   : Step::FinalKliker;
      break;
    case Step::FinalKliker:
      if (!kliker(joystick)) {
        flow = Flow::Yield;
        break;
      }
      off();
      closeScreen(0);
      flow = hold(SCREEN_CLOSE_VBLS, Step::CloseDancer);
      break;
    case Step::CloseDancer:
      closeScreen(1);
      m_count = FULL_VOLUME;
      flow = hold(SCREEN_CLOSE_VBLS, Step::MusicFade);
      break;
    case Step::MusicFade:
      flow = musicFade();
      break;
    case Step::Finished:
      flow = Flow::Yield;
      break;
    }
  }
}

void EndingScene::start() {
  if (!m_session.bossExit) {
    throw std::logic_error("EndingScene needs the screen the boss stage left");
  }
  m_stage.emplace(std::move(*m_session.bossExit));
  m_session.bossExit.reset();
  m_panelTop = m_stage->panelY;
  if (!m_stage->buffer.isAutobacking()) {
    m_stage->buffer.autoback([](IndexedSurface &surface) { surface.fill(0); });
  }
  stageFrame();
  m_stageShown = true;
  m_panelShown = true;
  m_border = STAGE_BORDER;
}

void EndingScene::era() {
  m_credits = m_host.loadEndingCredits();
  m_panel = std::make_unique<StatusPanel>(
      m_host.loadPanelPicture(StreetStage::LOADING_STRIP), Picture{});
  m_host.stopMusic();
  m_images.clear();
  m_parked.clear();
  m_loading.queue([this] {
    m_images.load(FIRST_IMAGE, m_host.loadSpriteSet(DANCE_SET, 0));
    m_parked = std::move(m_images);
    m_images.clear();
  });
  m_loading.queue([this] {
    m_images.load(FIRST_IMAGE, m_host.loadSpriteSet(STILL_SET, 0));
  });
  m_loading.queue([this] {
    m_picture = m_host.loadPicture(STILL_PICTURE);
    m_picturePalette = m_host.loadPalette(STILL_PICTURE);
    m_picturePalette.resize(STILL_COLORS, BLACK);
  });
  m_loading.queue([this] { m_host.loadMusic(ENDING_TUNE); });
  m_step = Step::Loading;
}

void EndingScene::fotoWhite() {
  openScreen(0, STILL_TOP, STILL_HEIGHT,
             effects::AmigaPalette(STILL_COLORS, WHITE));
  m_screens[0].surface.unpack(m_picture, 0, 0);
  m_picture = Picture{};
  m_bobScreen = 0;
  m_border = BLACK;
}

void EndingScene::hideStill() {
  effects::AmigaPalette palette = m_screens[0].palette;
  palette[0] = STILL_GREY;
  openScreen(1, STILL_TOP, STILL_HEIGHT, std::move(palette));
  m_screens[1].hidden = true;
  m_machine.bind(TEXT_BOX, &m_bobs.object(TEXT_BOX));
  m_machine.bind(WALKER, &m_bobs.object(WALKER));
}

void EndingScene::farewell() {
  Screen &still = m_screens[0];
  m_border = still.palette[0];
  BobLayer::paste(still.surface, m_images, DOORWAY_X, DOORWAY_Y, DOORWAY_IMAGE);
  BobLayer::paste(still.surface, m_images, FAREWELL_X, FAREWELL_Y,
                  FAREWELL_IMAGE);
  m_bobs.set(WALKER, WALKER_X, WALKER_Y, WALKER_IMAGE);
  m_machine.create(WALKER, amal::actors::walkAway());
  m_machine.startAll();
}

void EndingScene::dance() {
  for (int bob = DANCER; bob <= DANCER + PORTRAITS; ++bob) {
    m_machine.bind(bob, &m_bobs.object(bob));
  }
  m_bobs.set(DANCER, DANCER_X, DANCER_Y, DANCER_IMAGE);
  for (int portrait = 1; portrait <= PORTRAITS; ++portrait) {
    m_bobs.set(DANCER + portrait, PORTRAIT_X, PORTRAIT_Y, portrait);
  }
  m_machine.create(DANCER, amal::actors::breakDance());
  for (int portrait = 1; portrait <= PORTRAITS; ++portrait) {
    m_machine.create(DANCER + portrait,
                     amal::actors::portraitEntrance(portrait));
  }
  m_machine.startAll();
}

void EndingScene::secondDance() {
  m_machine.create(DANCER, amal::actors::danceFinale());
  for (int portrait = 1; portrait <= PORTRAITS; ++portrait) {
    m_machine.create(DANCER + portrait,
                     amal::actors::portraitShuttle(portrait));
  }
  m_machine.startAll();
}

void EndingScene::textScreen() {
  effects::AmigaPalette palette(TEXT_COLORS, DEFAULT_COLOR);
  std::fill_n(palette.begin(), 3, BLACK);
  openScreen(0, TEXT_TOP, TEXT_HEIGHT, std::move(palette));
  m_border = m_screens[0].palette[0];
}

void EndingScene::font(const std::string &text, int y) {
  const int length = static_cast<int>(text.size());
  const int x = (LINE_WIDTH - length * GLYPH_WIDTH) / 2;
  for (int i = 1; i <= length; ++i) {
    const int image =
        static_cast<unsigned char>(text[static_cast<std::size_t>(i - 1)]) +
        GLYPH_OFFSET;
    BobLayer::paste(m_screens[0].surface, m_images, i * GLYPH_WIDTH + x, y,
                    image);
  }
}

void EndingScene::pageUp() {
  if (m_page == SECOND_DANCE_PAGE) {
    secondDance();
  }
  for (const CreditLine &line :
       m_credits.pages[static_cast<std::size_t>(m_page)].lines) {
    font(line.text, line.y);
  }
  m_fader.start(m_screens[0].palette, BEAT_SPEED, beat(INK, SHADE));
}

EndingScene::Flow EndingScene::musicFade() {
  if (m_count >= 0) {
    m_host.setMusicVolume(m_count);
    --m_count;
    return Flow::Yield;
  }
  m_host.stopMusic();
  m_host.setMusicVolume(FULL_VOLUME);
  m_session.stageReached = m_session.registers[RO];
  m_step = Step::Finished;
  return Flow::Yield;
}

void EndingScene::off() {
  m_machine.destroyAll();
  m_bobs.offAll();
}

void EndingScene::openScreen(int number, int top, int height,
                             effects::AmigaPalette palette) {
  Screen &screen = m_screens[static_cast<std::size_t>(number)];
  screen.open = true;
  screen.hidden = false;
  screen.top = effects::pictureLine(top, m_ntsc);
  screen.surface = IndexedSurface(WIDTH, height);
  screen.palette = std::move(palette);
  if (number == 0) {
    m_fader = effects::PaletteFader{};
  }
}

void EndingScene::closeScreen(int number) {
  m_screens[static_cast<std::size_t>(number)] = Screen{};
  if (number == 0) {
    m_fader = effects::PaletteFader{};
  }
  if (number == 1) {
    m_dancerBuffer.reset();
  }
}

void EndingScene::redraw() {
  const Screen &screen = m_screens[static_cast<std::size_t>(m_bobScreen)];
  if (!screen.open || (m_bobScreen == 1 && m_dancerBuffer)) {
    return;
  }
  m_display = screen.surface;
  m_bobs.draw(m_display, m_images);
}

} // namespace openfranko::src::engine::street
