#ifndef SYSTEMS_JAGUAR_TEXTPANEL_H_
#define SYSTEMS_JAGUAR_TEXTPANEL_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {

inline constexpr uint16_t PANEL_INK = 0xFFFF;
inline constexpr uint16_t PANEL_PAPER = 0x0001;

void drawPanelLine(uint16_t *pixels, int width, int line, const char *text,
                   uint16_t ink = PANEL_INK, uint16_t paper = PANEL_PAPER);
void drawPanelCell(uint16_t *pixels, int width, int line, int column,
                   char character, uint16_t ink = PANEL_INK,
                   uint16_t paper = PANEL_PAPER);

} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_TEXTPANEL_H_
