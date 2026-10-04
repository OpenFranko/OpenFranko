#include "EndingScene.h"

#include "../../../systems/audio/Mixer.h"
#include "../../AmigaDisplay.h"
#include "../../effects/sequences/BlyskSequence.h"
#include "../../effects/sequences/FotoSequence.h"
#include "../actors/compiled/CompiledActors.h"
#include "../core/Font.h"
#include "../ui/StageFrame.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace openfranko::src::engine::street::scenes {
namespace {

constexpr int DANCE_SET = 0x38;
constexpr int STILL_SET = 0x37;
constexpr int STILL_PICTURE = 0x3BD;
constexpr int ENDING_TUNE = 0x25F;

constexpr effects::color::AmigaColor DEFAULT_COLOR = 0x000;
constexpr effects::color::AmigaColor STILL_GREY = 0x444;

constexpr int STAGE_WIDTH = 304;

constexpr int KLIKER_FRAMES = 2000;
constexpr int VERSION12_FINAL_KLIKER_FRAMES = 10000;
constexpr int16_t KLIKER_FIRE = 16;

constexpr int STILL_TOP = 50;
constexpr int STILL_HEIGHT = 256;
constexpr std::size_t STILL_COLORS = 32;
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
constexpr int BEAT_SPEED = 10;
constexpr int BEAT_HOLD = 150;
constexpr int BEAT_DARK = 150;
constexpr std::size_t GLYPHS_PER_FRAME = 4;

const effects::color::AmigaPalette DANCER_PALETTE = {
    0x000, 0x06F, 0x730, 0x840, 0x950, 0xA60, 0xB70, 0xC80,
    0xD90, 0xEA0, 0xFB0, 0xFC1, 0xFD2, 0xFE3, 0xFF4, 0xFFF};

const systems::graphics::Layer BLANK_LAYER{};

effects::color::AmigaPalette beat(effects::color::AmigaColor ink,
                                  effects::color::AmigaColor shade) {
  effects::color::AmigaPalette target(TEXT_COLORS,
                                      effects::color::PaletteFader::KEEP);
  target[0] = effects::color::BLACK;
  target[1] = ink;
  target[2] = shade;
  return target;
}

} // namespace

EndingScene::EndingScene(StreetHost &host, session::GameSession &session,
                         bool ntsc)
    : m_host(host), m_session(session), m_machine(session.registers),
      m_display(0, 0), m_ntsc(ntsc),
      m_displayLine(pictureLine(DISPLAY_LINE, ntsc)) {}

void EndingScene::showSprites(bool stills, bool dancers) {
  m_sprites = stills;
  m_dancerSprites = dancers;
  if (m_dancerBuffer) {
    m_dancerBuffer->setSprites(dancers);
  }
}

void EndingScene::advance(int16_t joystick) {
  if (m_step == Step::Finished) {
    return;
  }
  stepCredits();
  if (m_stage) {
    stageFrame();
  }
  if (m_dancerBuffer) {
    m_dancerBuffer->vbl();
  }
  if (m_dancerCopper) {
    m_screens[1].hidden = false;
    m_dancerCopper = false;
  }
  m_stillVbl = true;
  m_machine.tick();
  const bool faded = m_fader.advance(m_screens[0].palette);
  if (!m_hold.holdsAtStart(m_frame)) {
    stillTest();
    if (m_dancerBuffer) {
      m_dancerBuffer->test(m_bobs, m_images);
    }
  }
  runBasic(joystick);
  if (!m_hold.holdsAtEnd(m_frame)) {
    stillTest();
    if (m_dancerBuffer) {
      m_dancerBuffer->test(m_bobs, m_images);
    }
  }
  if (!faded) {
    preparePage();
  }
  redraw();
  ++m_frame;
}

void EndingScene::compose(std::vector<uint32_t> &frame) const {
  systems::graphics::rasterize(output(), frame);
}

systems::graphics::Display EndingScene::output() const {
  systems::graphics::Display display;
  output(display);
  return display;
}

systems::graphics::Display EndingScene::upcomingOutput() const {
  systems::graphics::Display display;
  upcomingOutput(display);
  return display;
}

void EndingScene::output(systems::graphics::Display &display) const {
  buildOutput(display, false);
}

void EndingScene::upcomingOutput(systems::graphics::Display &display) const {
  buildOutput(display, m_step != Step::Finished);
}

void EndingScene::buildOutput(systems::graphics::Display &display,
                              bool upcoming) const {
  display.width = SCREEN_WIDTH;
  display.height = SCREEN_HEIGHT;
  display.displayHeight = SCREEN_HEIGHT;
  display.border = m_border;
  display.revision = 0;
  std::size_t used = 0;
  const auto next = [&display, &used]() -> systems::graphics::Layer & {
    if (used == display.layers.size()) {
      display.layers.emplace_back();
    }
    systems::graphics::Layer &layer = display.layers[used++];
    layer = BLANK_LAYER;
    return layer;
  };
  if (m_stageShown && m_stage) {
    const core::IndexedSurface &shown =
        upcoming ? m_stage->buffer.upcoming() : m_stage->buffer.shown();
    const int rowsPerLine = m_stage->laced ? 2 : 1;
    systems::graphics::Layer &stage = next();
    stage.pixels = shown.pixels().data();
    stage.stride = shown.width();
    stage.sourceColumns = shown.width();
    stage.sourceRows = shown.height();
    stage.sourceX = m_stage->offsetX;
    stage.sourceY = (m_displayLine - m_stage->displayY) * rowsPerLine;
    stage.sourceStep = rowsPerLine;
    stage.columns = STAGE_WIDTH;
    stage.rows = SCREEN_HEIGHT;
    stage.mask = static_cast<uint8_t>(m_stage->palette.size() - 1);
    stage.palette = m_stage->palette;
    stage.revision = shown.revision();
  }
  if (const core::IndexedSurface *shown = panel()) {
    systems::graphics::Layer &layer = next();
    layer.pixels = shown->pixels().data();
    layer.stride = shown->width();
    layer.sourceColumns = shown->width();
    layer.sourceRows = ui::StatusPanel::VISIBLE_HEIGHT;
    layer.top = m_panelTop - m_displayLine;
    layer.columns = ui::StatusPanel::WIDTH;
    layer.rows = ui::StatusPanel::VISIBLE_HEIGHT;
    layer.palette = ui::panelPalette();
    layer.revision = shown->revision();
  }
  for (int number : {1, 0}) {
    const Screen &screen = m_screens[static_cast<std::size_t>(number)];
    const bool unhiding = upcoming && number == 1 && m_dancerCopper;
    if (!screen.open || (screen.hidden && !unhiding)) {
      continue;
    }
    systems::graphics::Layer &layer = next();
    const core::IndexedSurface *dancer = nullptr;
    if (number == 1 && m_dancerBuffer) {
      const core::DoubleBuffer::View view = upcoming
                                                ? m_dancerBuffer->upcomingView()
                                                : m_dancerBuffer->shownView();
      dancer = &view.pixels;
      layer.carriesSprites = m_dancerSprites;
      layer.sprites = view.sprites;
    } else if (number == m_bobScreen && m_stillSprited) {
      layer.carriesSprites = true;
      layer.sprites = m_stillSprites;
    }
    const core::IndexedSurface &surface =
        dancer ? *dancer
               : (number == m_bobScreen && !m_stillSprited ? m_display.shown()
                                                           : screen.surface);
    layer.pixels = surface.pixels().data();
    layer.stride = surface.width();
    layer.sourceColumns = surface.width();
    layer.sourceRows = surface.height();
    layer.top = screen.top - m_displayLine;
    layer.columns = std::min(SCREEN_WIDTH, surface.width());
    layer.rows = surface.height();
    layer.mask = static_cast<uint8_t>(screen.palette.size() - 1);
    layer.palette = screen.palette;
    layer.revision = surface.revision();
  }
  display.layers.resize(used);
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

effects::color::AmigaColor EndingScene::border() const { return m_border; }

const core::IndexedSurface &EndingScene::preparedPage() const {
  return m_nextPage;
}

const core::IndexedSurface &EndingScene::screen(int number) const {
  return m_screens[static_cast<std::size_t>(number)].surface;
}

const effects::color::AmigaPalette &EndingScene::palette(int number) const {
  return m_screens[static_cast<std::size_t>(number)].palette;
}

const core::BobLayer &EndingScene::bobs() const { return m_bobs; }

const core::IndexedSurface *EndingScene::panel() const {
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
  m_hold.start(m_frame, frames);
  return wait(frames, next);
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

bool EndingScene::kliker(int16_t joystick, int frames) {
  const bool pressed = m_session.version == GameVersion::V12
                           ? (joystick & KLIKER_FIRE) != 0
                           : joystick > 0;
  if (++m_count <= frames && !pressed) {
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
      flow = hold(SCREEN_CLOSE_SHOWN_VBLS, Step::StageGone);
      break;
    case Step::StageGone:
      m_stageShown = false;
      flow = hold(SCREEN_CLOSE_HIDDEN_VBLS, Step::ClosePanel);
      break;
    case Step::ClosePanel:
      flow = hold(SCREEN_CLOSE_SHOWN_VBLS, Step::PanelGone);
      break;
    case Step::PanelGone:
      m_panelShown = false;
      m_stage.reset();
      flow = hold(SCREEN_CLOSE_HIDDEN_VBLS, Step::Foto);
      break;
    case Step::Foto:
      m_host.setMusicVolume(systems::audio::Mixer::FULL_VOLUME);
      m_host.playMusic();
      flow = hold(effects::sequences::FotoSequence::FOTO_OPEN_VBLS,
                  Step::FotoWhite);
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
      flow = kliker(joystick, KLIKER_FRAMES) ? wait(STILL_HOLD, Step::StillOff)
                                             : Flow::Yield;
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
      if (!kliker(joystick, KLIKER_FRAMES)) {
        flow = Flow::Yield;
        break;
      }
      m_screens[0].surface.fill(0);
      m_fader.start(
          m_screens[0].palette, STILL_FADE_SPEED,
          effects::color::AmigaPalette(STILL_COLORS, effects::color::BLACK));
      flow = wait(STILL_FADE_WAIT, Step::CloseStill);
      break;
    case Step::CloseStill:
      off();
      flow = hold(SCREEN_CLOSE_SHOWN_VBLS, Step::StillGone);
      break;
    case Step::StillGone:
      closeScreen(0);
      flow = hold(SCREEN_CLOSE_HIDDEN_VBLS, Step::CloseHidden);
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
      m_screens[1].hidden = true;
      m_dancerBuffer.emplace(m_screens[1].surface);
      m_dancerBuffer->setSprites(m_dancerSprites);
      m_bobScreen = 1;
      flow = hold(DOUBLE_BUFFER_VBLS, Step::Dance);
      break;
    case Step::Dance:
      dance();
      m_dancerCopper = true;
      flow = hold(SCREEN_OPEN_VBLS, Step::TextScreen);
      break;
    case Step::TextScreen:
      textScreen();
      m_page = 0;
      m_count = 0;
      finishCredits();
      m_step = m_credits.pages.empty() ? Step::FinalKliker : Step::Page;
      break;
    case Step::Page:
      pageUp();
      flow = wait(BEAT_HOLD +
                      m_credits.pages[static_cast<std::size_t>(m_page)].beat,
                  Step::PageFade);
      break;
    case Step::PageFade:
      m_fader.start(m_screens[0].palette, BEAT_SPEED,
                    beat(effects::color::BLACK, effects::color::BLACK));
      flow = wait(BEAT_DARK, Step::PageClear);
      break;
    case Step::PageClear:
      m_pagePasted =
          m_preparedPage == m_page + 1 && m_pastedGlyphs == m_pageGlyphs.size();
      if (m_pagePasted) {
        m_screens[0].surface.copy(m_nextPage, 0, 0, m_nextPage.width(),
                                  m_nextPage.height(), 0, 0);
      } else {
        m_screens[0].surface.fill(0);
      }
      ++m_page;
      m_count = 0;
      m_step = m_page < static_cast<int>(m_credits.pages.size())
                   ? Step::Page
                   : Step::FinalKliker;
      break;
    case Step::FinalKliker:
      if (!kliker(joystick, m_session.version == GameVersion::V12
                                ? VERSION12_FINAL_KLIKER_FRAMES
                                : KLIKER_FRAMES)) {
        flow = Flow::Yield;
        break;
      }
      off();
      flow = hold(SCREEN_CLOSE_SHOWN_VBLS, Step::TextGone);
      break;
    case Step::TextGone:
      closeScreen(0);
      flow = hold(SCREEN_CLOSE_HIDDEN_VBLS, Step::CloseDancer);
      break;
    case Step::CloseDancer:
      flow = hold(SCREEN_CLOSE_SHOWN_VBLS, Step::DancerGone);
      break;
    case Step::DancerGone:
      closeScreen(1);
      m_count = systems::audio::Mixer::FULL_VOLUME;
      flow = hold(SCREEN_CLOSE_HIDDEN_VBLS, Step::MusicFade);
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
    m_stage->buffer.autoback(
        [](core::IndexedSurface &surface) { surface.fill(0); });
  }
  stageFrame();
  m_stageShown = true;
  m_panelShown = true;
  m_border = ui::STAGE_BORDER;
}

void EndingScene::stepCredits() {
  if (m_creditsLoad && m_creditsLoad->step(m_credits)) {
    m_creditsLoad.reset();
  }
}

void EndingScene::finishCredits() {
  while (m_creditsLoad) {
    stepCredits();
  }
}

void EndingScene::era() {
  m_credits = core::EndingCredits{};
  m_creditsLoad = m_host.beginEndingCredits();
  m_panel = std::make_unique<ui::StatusPanel>(
      m_host.loadPanelPicture(StreetHost::LOADING_STRIP), core::Picture{},
      m_session.version);
  m_host.stopMusic();
  m_images.clear();
  m_parked.clear();
  m_loading.queueSteps([this, load = spriteSetJob(
                                  m_host, m_images, [] { return DANCE_SET; }, 0,
                                  core::ImageBank::FIRST_IMAGE,
                                  ui::LoadingQueue::FILE_FRAMES)]() mutable {
    if (!load()) {
      return false;
    }
    m_parked = std::move(m_images);
    m_images.clear();
    return true;
  });
  m_loading.queueSteps(spriteSetJob(
      m_host, m_images, [] { return STILL_SET; }, 0,
      core::ImageBank::FIRST_IMAGE, ui::LoadingQueue::FILE_FRAMES));
  m_loading.queueSteps([this, load = pictureJob(
                                  m_host, [] { return STILL_PICTURE; },
                                  m_picture, &m_picturePalette)]() mutable {
    if (!load()) {
      return false;
    }
    m_picturePalette.resize(STILL_COLORS, effects::color::BLACK);
    return true;
  });
  m_loading.queueSteps(musicJob(
      m_host, [] { return ENDING_TUNE; }, ui::LoadingQueue::FILE_FRAMES));
  m_step = Step::Loading;
}

void EndingScene::fotoWhite() {
  openScreen(0, STILL_TOP, STILL_HEIGHT,
             effects::color::AmigaPalette(STILL_COLORS, effects::color::WHITE));
  m_screens[0].surface.unpack(m_picture, 0, 0);
  m_picture = core::Picture{};
  m_bobScreen = 0;
  m_border = effects::color::BLACK;
}

void EndingScene::hideStill() {
  effects::color::AmigaPalette palette = m_screens[0].palette;
  palette[0] = STILL_GREY;
  openScreen(1, STILL_TOP, STILL_HEIGHT, std::move(palette));
  m_screens[1].hidden = true;
  m_machine.bind(TEXT_BOX, &m_bobs.object(TEXT_BOX));
  m_machine.bind(WALKER, &m_bobs.object(WALKER));
}

void EndingScene::farewell() {
  Screen &still = m_screens[0];
  m_border = still.palette[0];
  core::BobLayer::paste(still.surface, m_images, DOORWAY_X, DOORWAY_Y,
                        DOORWAY_IMAGE);
  core::BobLayer::paste(still.surface, m_images, FAREWELL_X, FAREWELL_Y,
                        FAREWELL_IMAGE);
  m_bobs.set(WALKER, WALKER_X, WALKER_Y, WALKER_IMAGE);
  m_machine.create(WALKER, actors::compiled::walkAway());
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
  m_machine.create(DANCER, actors::compiled::breakDance());
  for (int portrait = 1; portrait <= PORTRAITS; ++portrait) {
    m_machine.create(DANCER + portrait,
                     actors::compiled::portraitEntrance(portrait));
  }
  m_machine.startAll();
}

void EndingScene::secondDance() {
  m_machine.create(DANCER, actors::compiled::danceFinale());
  for (int portrait = 1; portrait <= PORTRAITS; ++portrait) {
    m_machine.create(DANCER + portrait,
                     actors::compiled::portraitShuttle(portrait));
  }
  m_machine.startAll();
}

void EndingScene::textScreen() {
  effects::color::AmigaPalette palette(TEXT_COLORS, DEFAULT_COLOR);
  std::fill_n(palette.begin(), 3, effects::color::BLACK);
  openScreen(0, TEXT_TOP, TEXT_HEIGHT, std::move(palette));
  m_border = m_screens[0].palette[0];
}

void EndingScene::pageUp() {
  if (m_page == SECOND_DANCE_PAGE) {
    secondDance();
  }
  if (!m_pagePasted) {
    for (const core::CreditLine &line :
         m_credits.pages[static_cast<std::size_t>(m_page)].lines) {
      core::font(line.text, line.y, [this](int x, int y, int image) {
        core::BobLayer::paste(m_screens[0].surface, m_images, x, y, image);
      });
    }
  }
  m_fader.start(m_screens[0].palette, BEAT_SPEED,
                beat(effects::sequences::BlyskSequence::INK,
                     effects::sequences::BlyskSequence::SHADE));
}

void EndingScene::preparePage() {
  const int next = m_page + 1;
  if (!isShowingCredits() || next >= static_cast<int>(m_credits.pages.size())) {
    return;
  }
  const core::IndexedSurface &text = m_screens[0].surface;
  if (m_preparedPage != next) {
    m_preparedPage = next;
    m_pastedGlyphs = 0;
    m_pageGlyphs.clear();
    for (const core::CreditLine &line :
         m_credits.pages[static_cast<std::size_t>(next)].lines) {
      core::font(line.text, line.y, [this](int x, int y, int image) {
        m_pageGlyphs.push_back({x, y, image});
      });
    }
    if (m_nextPage.width() != text.width() ||
        m_nextPage.height() != text.height()) {
      m_nextPage = core::IndexedSurface(text.width(), text.height());
    } else {
      m_nextPage.fill(0);
    }
    return;
  }
  for (std::size_t pasted = 0;
       pasted < GLYPHS_PER_FRAME && m_pastedGlyphs < m_pageGlyphs.size();
       ++pasted, ++m_pastedGlyphs) {
    const PageGlyph &glyph = m_pageGlyphs[m_pastedGlyphs];
    core::BobLayer::paste(m_nextPage, m_images, glyph.x, glyph.y, glyph.image);
  }
}

EndingScene::Flow EndingScene::musicFade() {
  if (m_count >= 0) {
    m_host.setMusicVolume(m_count);
    --m_count;
    return Flow::Yield;
  }
  m_host.stopMusic();
  m_host.setMusicVolume(systems::audio::Mixer::FULL_VOLUME);
  m_session.border = m_border;
  m_step = Step::Finished;
  return Flow::Yield;
}

void EndingScene::off() {
  m_machine.destroyAll();
  m_bobs.offAll();
}

void EndingScene::openScreen(int number, int top, int height,
                             effects::color::AmigaPalette palette) {
  Screen &screen = m_screens[static_cast<std::size_t>(number)];
  screen.open = true;
  screen.hidden = false;
  screen.top = pictureLine(top, m_ntsc);
  screen.surface = core::IndexedSurface(SCREEN_WIDTH, height);
  screen.palette = std::move(palette);
  if (number == 0) {
    m_fader = effects::color::PaletteFader{};
  }
}

void EndingScene::closeScreen(int number) {
  m_screens[static_cast<std::size_t>(number)] = Screen{};
  if (number == 0) {
    m_fader = effects::color::PaletteFader{};
  }
  if (number == 1) {
    m_dancerBuffer.reset();
  }
}

void EndingScene::redraw() {
  const Screen &screen = m_screens[static_cast<std::size_t>(m_bobScreen)];
  m_stillSprited = false;
  if (!screen.open || (m_bobScreen == 1 && m_dancerBuffer)) {
    return;
  }
  if (m_sprites &&
      m_stillBobs.sprites(screen.surface, m_images, m_stillSprites)) {
    m_stillSprited = true;
    return;
  }
  core::IndexedSurface &display = m_display.compose();
  display = screen.surface;
  m_stillBobs.draw(display, m_images);
}

void EndingScene::stillTest() {
  if (m_stillVbl) {
    m_stillVbl = false;
    m_stillBobs = m_bobs;
  }
}

} // namespace openfranko::src::engine::street::scenes
