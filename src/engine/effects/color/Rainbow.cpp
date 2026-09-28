#include "Rainbow.h"

#include <array>
#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace openfranko::src::engine::effects {
namespace {

struct Move {
  int speed;
  int step;
  int count;
};

struct Channel {
  std::vector<Move> moves;
  std::size_t position = 0;
  int value = 0;
  int plus = 0;
  int countdown = 1;
  int speed = 0;
  int remaining = 1;
};

class Reader {
public:
  explicit Reader(const std::string &text) : m_text(text) {}

  bool atEnd() {
    skipSpaces();
    return m_index >= m_text.size();
  }

  void expect(char wanted) {
    skipSpaces();
    if (m_index >= m_text.size() || m_text[m_index] != wanted) {
      throw std::invalid_argument("Set Rainbow: syntax error in \"" + m_text +
                                  "\"");
    }
    ++m_index;
  }

  int number() {
    skipSpaces();
    const std::size_t start = m_index;
    if (m_index < m_text.size() &&
        (m_text[m_index] == '-' || m_text[m_index] == '+')) {
      ++m_index;
    }
    while (m_index < m_text.size() &&
           std::isdigit(static_cast<unsigned char>(m_text[m_index]))) {
      ++m_index;
    }
    if (m_index == start ||
        !std::isdigit(static_cast<unsigned char>(m_text[m_index - 1]))) {
      throw std::invalid_argument("Set Rainbow: number expected in \"" +
                                  m_text + "\"");
    }
    return std::stoi(m_text.substr(start, m_index - start));
  }

private:
  void skipSpaces() {
    while (m_index < m_text.size() && m_text[m_index] == ' ') {
      ++m_index;
    }
  }

  const std::string &m_text;
  std::size_t m_index = 0;
};

std::vector<Move> parse(const std::string &program) {
  Reader reader(program);
  std::vector<Move> moves;
  while (!reader.atEnd()) {
    reader.expect('(');
    Move move{};
    move.speed = reader.number();
    reader.expect(',');
    move.step = reader.number();
    reader.expect(',');
    move.count = reader.number();
    reader.expect(')');
    if (move.speed <= 0 || move.count < 0) {
      throw std::invalid_argument("Set Rainbow: bad move in \"" + program +
                                  "\"");
    }
    moves.push_back(move);
  }
  if (moves.empty()) {
    moves.push_back(Move{0, 0, 0});
  }
  return moves;
}

void step(Channel &channel) {
  if (channel.countdown == 0 || --channel.countdown != 0) {
    return;
  }
  channel.countdown = channel.speed;
  channel.value = (channel.value + channel.plus) & 0xF;
  if (channel.remaining == 0 || --channel.remaining != 0) {
    return;
  }
  if (channel.position >= channel.moves.size()) {
    channel.position = 0;
  }
  const Move &move = channel.moves[channel.position++];
  channel.countdown = move.speed;
  channel.speed = move.speed;
  channel.plus = move.step;
  channel.remaining = move.count;
}

} // namespace

AmigaPalette rainbowTable(int height, const std::string &red,
                          const std::string &green, const std::string &blue,
                          AmigaColor start) {
  std::array<Channel, 3> channels;
  const std::array<const std::string *, 3> programs = {&red, &green, &blue};
  for (std::size_t i = 0; i < channels.size(); ++i) {
    channels[i].moves = parse(*programs[i]);
    channels[i].value = (start >> (8 - 4 * static_cast<int>(i))) & 0xF;
  }
  AmigaPalette table;
  table.reserve(static_cast<std::size_t>(height));
  for (int entry = 0; entry < height; ++entry) {
    for (Channel &channel : channels) {
      step(channel);
    }
    table.push_back(static_cast<AmigaColor>(
        channels[0].value << 8 | channels[1].value << 4 | channels[2].value));
  }
  return table;
}

} // namespace openfranko::src::engine::effects
