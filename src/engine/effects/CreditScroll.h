#ifndef ENGINE_EFFECTS_CREDITSCROLL_H_
#define ENGINE_EFFECTS_CREDITSCROLL_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

class CreditScroll {
public:
  CreditScroll(int16_t y, int firstImage, int lastImage);

  void advance();

  int16_t y() const;
  int image() const;

private:
  int16_t m_y;
  int m_image;
  int m_firstImage;
  int m_lastImage;
  bool m_started = false;
  bool m_liftNext = false;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_CREDITSCROLL_H_
