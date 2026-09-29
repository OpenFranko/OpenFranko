#include "MirageState.h"

namespace openfranko::src::engine::states::mirage {
namespace {

constexpr auto PICTURE_PATH = "assets/03C3.bmp";

constexpr int DISPLAY_LINE = 30;

constexpr effects::sequences::FotoSequence ::Timings TIMINGS{5, 200, 5, 70,
                                                             true};

} // namespace

MirageState::MirageState(systems::graphics::VideoSystem &videoSystem)
    : m_foto(videoSystem, PICTURE_PATH, DISPLAY_LINE, TIMINGS) {}

std::optional<EngineStateId> MirageState::update() {
  if (m_foto.sequence().isFinished()) {
    return EngineStateId::WorldSoftware;
  }

  m_foto.advance();
  return std::nullopt;
}

} // namespace openfranko::src::engine::states::mirage
