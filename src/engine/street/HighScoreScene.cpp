#include "HighScoreScene.h"

#include "StageFrame.h"

#include <array>
#include <cctype>
#include <cstddef>
#include <utility>

namespace openfranko::src::engine::street {
namespace {

constexpr int RN = 13;

constexpr int MENU_TUNE = 0x261;
constexpr int TITLE = 0x3BA;
constexpr int PICTURE = 0x3B9;
constexpr int LETTER_SET = 0x35;
constexpr int LETTER_SAMPLES = 5;
constexpr int FIRST_IMAGE = 1;

constexpr int FULL_VOLUME = 63;
constexpr int MUSIC_START_WAIT = 2;

constexpr std::size_t COLORS = 32;
constexpr effects::AmigaColor BLACK = 0x000;
constexpr int DIM_ROUNDS = 4;
constexpr int DIM_SPEED = 100;
constexpr int DIM_WAIT = 1;
constexpr int DIMMED_WAIT = 3;
constexpr int ROW_WAIT = 10;

struct Relit {
  std::size_t index;
  effects::AmigaColor color;
};

constexpr std::array<Relit, 3> RELIT = {{
    {29, 0x769},
    {30, 0xB95},
    {31, 0xFC0},
}};

constexpr int NAME_X = 56;
constexpr int LAST_CELL_X = 196;
constexpr int CELL_WIDTH = 10;
constexpr int CELL_HEIGHT = 15;
constexpr int FIRST_ROW_Y = 32;
constexpr int ROW_PITCH = 20;
constexpr int SCORE_RIGHT = 272;
constexpr int SCORE_CHARACTERS = 4;
constexpr int LETTER_IMAGE = 14;
constexpr int DIGIT_IMAGE_OFFSET = 44;
constexpr int TYPED_IMAGE_OFFSET = 51;
constexpr int CURSOR = 1;
constexpr int CURSOR_IMAGE = 40;
constexpr int CURSOR_DROP = 15;
constexpr int SCRATCH_COPY_WIDTH = 166;
constexpr int SCRATCH_WIDTH = SCRATCH_COPY_WIDTH & ~15;
constexpr int SCRATCH_HEIGHT = 20;

constexpr int HOLD_FRAMES = 300;
constexpr int FADE_SPEED = 2;
constexpr int FADE_WAIT = 30;

} // namespace

HighScoreScene::HighScoreScene(StreetHost &host, GameSession &session,
                               const effects::GameOptions &options, Save save)
    : m_host(host), m_session(session), m_options(options),
      m_save(std::move(save)), m_screen(WIDTH, HEIGHT),
      m_scratch(SCRATCH_WIDTH, SCRATCH_HEIGHT), m_display(WIDTH, HEIGHT),
      m_palette(COLORS, BLACK) {}

void HighScoreScene::advance(char key) {
  if (m_step == Step::Finished) {
    return;
  }
  m_fader.tick(m_palette);
  runBasic(key);
  redraw();
  ++m_frame;
}

void HighScoreScene::compose(std::vector<uint32_t> &frame) const {
  frame.assign(static_cast<std::size_t>(WIDTH * HEIGHT), toArgb(BLACK));
  if (!m_shown) {
    return;
  }
  const std::vector<uint8_t> &pixels = m_display.pixels();
  for (std::size_t i = 0; i < frame.size(); ++i) {
    frame[i] = toArgb(m_palette[pixels[i] & (COLORS - 1)]);
  }
}

HighScoreScene::Outcome HighScoreScene::outcome() const { return m_outcome; }

bool HighScoreScene::isShown() const { return m_shown; }

bool HighScoreScene::isEntering() const { return m_entering; }

int HighScoreScene::slot() const { return m_slot; }

int HighScoreScene::rowsShown() const { return m_rowsShown; }

const std::string &HighScoreScene::name() const { return m_name; }

const BobLayer &HighScoreScene::bobs() const { return m_bobs; }

const IndexedSurface &HighScoreScene::screen() const { return m_screen; }

const effects::AmigaPalette &HighScoreScene::palette() const {
  return m_palette;
}

HighScoreScene::Flow HighScoreScene::wait(int frames, Step next) {
  m_resumeFrame = m_frame + frames;
  m_step = next;
  return Flow::Yield;
}

void HighScoreScene::runBasic(char key) {
  Flow flow = Flow::Continue;
  while (flow == Flow::Continue && m_frame >= m_resumeFrame) {
    switch (m_step) {
    case Step::Reset:
      reset();
      break;
    case Step::Loading:
      if (m_loading.advance(nullptr)) {
        m_step = m_afterLoading;
      } else {
        flow = Flow::Yield;
      }
      break;
    case Step::MenuMusic:
      m_host.playMusic();
      m_host.setMusicVolume(m_options.music ? FULL_VOLUME : 0);
      flow = wait(MUSIC_START_WAIT, Step::Pictures);
      break;
    case Step::Pictures:
      queuePictures();
      break;
    case Step::Loaded:
      flow = loaded();
      break;
    case Step::Dim:
      flow = dim();
      break;
    case Step::Dimmed:
      flow = dimmed();
      break;
    case Step::Relight:
      relight();
      break;
    case Step::Row:
      flow = row();
      break;
    case Step::EntryStart:
      startEntry();
      break;
    case Step::Entry:
      flow = entry(key);
      break;
    case Step::Hold:
      flow = wait(HOLD_FRAMES, Step::FadeOut);
      break;
    case Step::FadeOut:
      m_fader.start(m_palette, FADE_SPEED,
                    effects::AmigaPalette(COLORS, BLACK));
      flow = wait(FADE_WAIT, Step::Clear);
      break;
    case Step::Clear:
      clear();
      flow = Flow::Yield;
      break;
    case Step::Finished:
      flow = Flow::Yield;
      break;
    }
  }
}

void HighScoreScene::reset() {
  amal::Registers &registers = m_session.registers;
  const int16_t kills = registers[RN];
  registers = GameSession::freshRegisters();
  registers[RN] = kills;
  m_host.stopMusic();
  m_loading.queue([this] { m_host.loadMusic(MENU_TUNE); });
  m_afterLoading = Step::MenuMusic;
  m_step = Step::Loading;
}

void HighScoreScene::queuePictures() {
  m_loading.queue([this] { m_host.loadPicture(TITLE); });
  m_loading.queue([this] {
    m_picture = m_host.loadPicture(PICTURE);
    m_picturePalette = m_host.loadPalette(PICTURE);
  });
  m_loading.queue([this] {
    m_images.load(FIRST_IMAGE,
                  m_host.loadSpriteSet(LETTER_SET, LETTER_SAMPLES));
  });
  m_afterLoading = Step::Loaded;
  m_step = Step::Loading;
}

HighScoreScene::Flow HighScoreScene::loaded() {
  const int kills = m_session.registers[RN];
  if (kills == 0) {
    m_outcome = Outcome::Menu;
    m_step = Step::Finished;
    return Flow::Yield;
  }
  m_slot = m_session.highScores.insert(kills);
  m_screen.unpack(m_picture, 0, 0);
  m_picture = Picture{};
  m_palette = m_picturePalette;
  m_palette.resize(COLORS, BLACK);
  m_round = 0;
  m_step = Step::Dim;
  return Flow::Continue;
}

HighScoreScene::Flow HighScoreScene::dim() {
  m_fader.start(m_palette, DIM_SPEED, effects::AmigaPalette(COLORS, BLACK));
  return wait(DIM_WAIT, Step::Dimmed);
}

HighScoreScene::Flow HighScoreScene::dimmed() {
  m_fader.start(m_palette, 1, m_palette);
  m_shown = true;
  ++m_round;
  return wait(DIMMED_WAIT, m_round < DIM_ROUNDS ? Step::Dim : Step::Relight);
}

void HighScoreScene::relight() {
  for (const Relit &relit : RELIT) {
    m_palette[relit.index] = relit.color;
  }
  m_row = HighScoreTable::ROWS - 1;
  m_step = Step::Row;
}

HighScoreScene::Flow HighScoreScene::row() {
  pasteRow(m_row);
  ++m_rowsShown;
  --m_row;
  if (m_row >= 0) {
    return wait(ROW_WAIT, Step::Row);
  }
  return wait(ROW_WAIT,
              m_slot < HighScoreTable::NO_SLOT ? Step::EntryStart : Step::Hold);
}

void HighScoreScene::pasteRow(int row) {
  const HighScoreTable &table = m_session.highScores;
  const int y = FIRST_ROW_Y + row * ROW_PITCH;
  for (int column = 0; column < HighScoreTable::NAME_LENGTH; ++column) {
    const int letter = table.letter(row, column);
    if (letter < HighScoreTable::LETTERS) {
      BobLayer::paste(m_screen, m_images, NAME_X + column * CELL_WIDTH, y,
                      letter + LETTER_IMAGE);
    }
  }
  const std::string score = " " + std::to_string(table.score(row)) + "   ";
  const int left = SCORE_RIGHT - CELL_WIDTH * static_cast<int>(score.size());
  for (int k = 1; k <= SCORE_CHARACTERS; ++k) {
    const char character = score[static_cast<std::size_t>(k - 1)];
    if (character > ' ') {
      BobLayer::paste(m_screen, m_images, left + k * CELL_WIDTH, y,
                      character - DIGIT_IMAGE_OFFSET);
    }
  }
}

void HighScoreScene::startEntry() {
  m_x = NAME_X;
  m_y = m_slot * ROW_PITCH + FIRST_ROW_Y;
  m_scratch.copy(m_screen, m_x, m_y, m_x + SCRATCH_COPY_WIDTH,
                 m_y + SCRATCH_HEIGHT, 0, 0);
  m_bobs.set(CURSOR, m_x, m_y + CURSOR_DROP, CURSOR_IMAGE);
  m_name.assign(HighScoreTable::NAME_LENGTH, ' ');
  m_entering = true;
  m_step = Step::Entry;
}

HighScoreScene::Flow HighScoreScene::entry(char key) {
  m_bobs.set(CURSOR, m_x, m_y + CURSOR_DROP, CURSOR_IMAGE);
  const char typed =
      static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
  const std::size_t cell =
      static_cast<std::size_t>((m_x - NAME_X) / CELL_WIDTH);
  if (typed >= 'A' && typed <= 'Z') {
    restoreCell();
    BobLayer::paste(m_screen, m_images, m_x, m_y, typed - TYPED_IMAGE_OFFSET);
    m_name[cell] = typed;
    if (m_x < LAST_CELL_X) {
      m_x += CELL_WIDTH;
    }
  } else if (typed == ' ') {
    if (m_x < LAST_CELL_X) {
      m_x += CELL_WIDTH;
    }
  } else if (typed == BACKSPACE) {
    restoreCell();
    m_name[cell] = ' ';
    if (m_x > NAME_X) {
      m_x -= CELL_WIDTH;
    }
  } else if (typed == RETURN) {
    commit();
    m_step = Step::Hold;
    return Flow::Continue;
  }
  return Flow::Yield;
}

void HighScoreScene::restoreCell() {
  const int from = m_x - NAME_X;
  m_screen.copy(m_scratch, from, 0, from + CELL_WIDTH, CELL_HEIGHT, m_x, m_y);
}

void HighScoreScene::commit() {
  m_session.highScores.setName(m_slot, m_name);
  m_session.textBuffer = m_name;
  m_bobs.offAll();
  m_entering = false;
  if (m_save) {
    m_save(m_session.highScores);
  }
}

void HighScoreScene::clear() {
  m_bobs.offAll();
  m_screen.fill(0);
  m_outcome = Outcome::Continue;
  m_step = Step::Finished;
}

void HighScoreScene::redraw() {
  if (!m_shown) {
    return;
  }
  m_display = m_screen;
  m_bobs.draw(m_display, m_images);
}

} // namespace openfranko::src::engine::street
