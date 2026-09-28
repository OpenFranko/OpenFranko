#include "CreditScroll.h"

namespace openfranko::src::engine::effects {
namespace {

constexpr int16_t TOP = -40;
constexpr int16_t BOTTOM = 350;
constexpr int IMAGE_STEP = 3;

} // namespace

CreditScroll::CreditScroll(int16_t y, int firstImage, int lastImage)
    : m_y(y), m_image(firstImage), m_firstImage(firstImage),
      m_lastImage(lastImage) {}

void CreditScroll::advance() {
  if (m_liftNext) {
    --m_y;
    m_liftNext = false;
    return;
  }

  if (m_started && m_y <= TOP) {
    m_y = BOTTOM;
    m_image += IMAGE_STEP;
    if (m_image > m_lastImage) {
      m_image = m_firstImage;
    }
  }
  m_started = true;
  m_liftNext = true;
}

int16_t CreditScroll::y() const { return m_y; }

int CreditScroll::image() const { return m_image; }

} // namespace openfranko::src::engine::effects
