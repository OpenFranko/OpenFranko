#include "AmalMotion.h"

#include <algorithm>
#include <cstdlib>
#include <utility>

namespace openfranko::src::engine::effects {
namespace {

constexpr uint16_t HALF = 0x8000;
constexpr int32_t LARGEST_STEP = 0xFFFF;

} // namespace

AmalMotion::AmalMotion(std::vector<Move> moves) : m_moves(std::move(moves)) {}

int16_t AmalMotion::advance(int16_t position) {
  if (m_framesLeft == 0) {
    if (m_next == m_moves.size()) {
      return position;
    }
    startNextMove();
  }

  const uint32_t accumulator =
      ((static_cast<uint32_t>(static_cast<uint16_t>(position)) << 16) |
       m_fraction) +
      static_cast<uint32_t>(m_step);
  m_fraction = static_cast<uint16_t>(accumulator & 0xFFFF);
  --m_framesLeft;
  return static_cast<int16_t>(accumulator >> 16);
}

bool AmalMotion::isFinished() const {
  return m_framesLeft == 0 && m_next == m_moves.size();
}

void AmalMotion::startNextMove() {
  const Move &move = m_moves[m_next++];
  const int frames = std::max<int>(move.frames, 1);
  int32_t perFrame = (std::abs(move.distance) << 8) / frames;
  if (perFrame > LARGEST_STEP) {
    perFrame = 0;
  }
  const auto word =
      static_cast<int16_t>(move.distance < 0 ? -perFrame : perFrame);
  m_step = static_cast<int32_t>(word) * 256;
  m_fraction = HALF;
  m_framesLeft = frames;
}

} // namespace openfranko::src::engine::effects
