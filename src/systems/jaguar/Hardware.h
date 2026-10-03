#ifndef SYSTEMS_JAGUAR_HARDWARE_H_
#define SYSTEMS_JAGUAR_HARDWARE_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {

inline constexpr uint32_t DRAM_END = 0x200000;
inline constexpr uint32_t CARTRIDGE = 0x800000;
inline constexpr uint32_t LEVEL0_VECTOR = 0x100;

inline constexpr uint32_t TOM = 0xF00000;
inline constexpr uint32_t MEMCON1 = TOM + 0x00;
inline constexpr uint32_t MEMCON2 = TOM + 0x02;
inline constexpr uint32_t HC = TOM + 0x04;
inline constexpr uint32_t VC = TOM + 0x06;
inline constexpr uint32_t OB0 = TOM + 0x10;
inline constexpr uint32_t OLP = TOM + 0x20;
inline constexpr uint32_t OBF = TOM + 0x26;
inline constexpr uint32_t VMODE = TOM + 0x28;
inline constexpr uint32_t BORD1 = TOM + 0x2A;
inline constexpr uint32_t BORD2 = TOM + 0x2C;
inline constexpr uint32_t HP = TOM + 0x2E;
inline constexpr uint32_t HBB = TOM + 0x30;
inline constexpr uint32_t HBE = TOM + 0x32;
inline constexpr uint32_t HS = TOM + 0x34;
inline constexpr uint32_t HVS = TOM + 0x36;
inline constexpr uint32_t HDB1 = TOM + 0x38;
inline constexpr uint32_t HDB2 = TOM + 0x3A;
inline constexpr uint32_t HDE = TOM + 0x3C;
inline constexpr uint32_t VP = TOM + 0x3E;
inline constexpr uint32_t VBB = TOM + 0x40;
inline constexpr uint32_t VBE = TOM + 0x42;
inline constexpr uint32_t VS = TOM + 0x44;
inline constexpr uint32_t VDB = TOM + 0x46;
inline constexpr uint32_t VDE = TOM + 0x48;
inline constexpr uint32_t VEB = TOM + 0x4A;
inline constexpr uint32_t VEE = TOM + 0x4C;
inline constexpr uint32_t VI = TOM + 0x4E;
inline constexpr uint32_t PIT0 = TOM + 0x50;
inline constexpr uint32_t PIT1 = TOM + 0x52;
inline constexpr uint32_t HEQ = TOM + 0x54;
inline constexpr uint32_t BG = TOM + 0x58;
inline constexpr uint32_t INT1 = TOM + 0xE0;
inline constexpr uint32_t INT2 = TOM + 0xE2;
inline constexpr uint32_t CLUT = TOM + 0x400;
inline constexpr int CLUT_ENTRIES = 256;

inline constexpr uint16_t INT1_VIDEO = 0x0001;
inline constexpr uint16_t INT1_GPU = 0x0002;
inline constexpr uint16_t INT1_OBJECT = 0x0004;
inline constexpr uint16_t INT1_TIMER = 0x0008;
inline constexpr uint16_t INT1_JERRY = 0x0010;
inline constexpr uint16_t INT1_CLEAR_SHIFT = 8;

inline constexpr uint16_t VMODE_ENABLE = 0x0001;
inline constexpr uint16_t VMODE_RGB16 = 0x0006;
inline constexpr uint16_t VMODE_CSYNC = 0x0040;
inline constexpr uint16_t VMODE_BGEN = 0x0080;
inline constexpr uint16_t VMODE_PWIDTH_SHIFT = 9;

inline constexpr uint16_t CONFIG_NTSC = 0x0010;

inline constexpr uint32_t GPU_FLAGS = TOM + 0x2100;
inline constexpr uint32_t GPU_END = TOM + 0x210C;
inline constexpr uint32_t GPU_PC = TOM + 0x2110;
inline constexpr uint32_t GPU_CTRL = TOM + 0x2114;
inline constexpr uint32_t GPU_DIVCTRL = TOM + 0x211C;
inline constexpr uint32_t GPU_RAM = TOM + 0x3000;
inline constexpr uint32_t GPU_RAM_SIZE = 0x1000;

inline constexpr uint32_t RISC_GO = 0x00000001;
inline constexpr uint32_t RISC_INT0 = 0x00000004;

inline constexpr uint32_t A1_BASE = TOM + 0x2200;
inline constexpr uint32_t A1_FLAGS = TOM + 0x2204;
inline constexpr uint32_t A1_CLIP = TOM + 0x2208;
inline constexpr uint32_t A1_PIXEL = TOM + 0x220C;
inline constexpr uint32_t A1_STEP = TOM + 0x2210;
inline constexpr uint32_t A1_FSTEP = TOM + 0x2214;
inline constexpr uint32_t A1_FPIXEL = TOM + 0x2218;
inline constexpr uint32_t A1_INC = TOM + 0x221C;
inline constexpr uint32_t A1_FINC = TOM + 0x2220;
inline constexpr uint32_t A2_BASE = TOM + 0x2224;
inline constexpr uint32_t A2_FLAGS = TOM + 0x2228;
inline constexpr uint32_t A2_MASK = TOM + 0x222C;
inline constexpr uint32_t A2_PIXEL = TOM + 0x2230;
inline constexpr uint32_t A2_STEP = TOM + 0x2234;
inline constexpr uint32_t B_CMD = TOM + 0x2238;
inline constexpr uint32_t B_COUNT = TOM + 0x223C;
inline constexpr uint32_t B_SRCD = TOM + 0x2240;
inline constexpr uint32_t B_DSTD = TOM + 0x2248;
inline constexpr uint32_t B_PATD = TOM + 0x2268;

inline constexpr uint32_t BLIT_IDLE = 0x00000001;
inline constexpr uint32_t BLIT_SRCEN = 0x00000001;
inline constexpr uint32_t BLIT_SRCENX = 0x00000004;
inline constexpr uint32_t BLIT_DSTEN = 0x00000008;
inline constexpr uint32_t BLIT_UPDA1 = 0x00000200;
inline constexpr uint32_t BLIT_UPDA2 = 0x00000400;
inline constexpr uint32_t BLIT_DSTA2 = 0x00000800;
inline constexpr uint32_t BLIT_PATDSEL = 0x00010000;
inline constexpr uint32_t BLIT_LFU_SOURCE = 0x01800000;
inline constexpr uint32_t BLIT_DCOMPEN = 0x08000000;
inline constexpr uint32_t BLIT_BKGWREN = 0x10000000;

inline constexpr uint32_t BLIT_PITCH1 = 0x00000000;
inline constexpr uint32_t BLIT_PIXEL8 = 0x00000018;
inline constexpr uint32_t BLIT_WIDTH_SHIFT = 9;
inline constexpr uint32_t BLIT_XADDPHR = 0x00000000;
inline constexpr uint32_t BLIT_XADDPIX = 0x00010000;
inline constexpr uint32_t BLIT_XADD0 = 0x00020000;
inline constexpr uint32_t BLIT_YADD1 = 0x00040000;
inline constexpr uint32_t BLIT_XSIGNSUB = 0x00080000;

inline constexpr uint32_t JERRY = 0xF10000;
inline constexpr uint32_t JPIT1 = JERRY + 0x0000;
inline constexpr uint32_t JPIT2 = JERRY + 0x0002;
inline constexpr uint32_t JPIT3 = JERRY + 0x0004;
inline constexpr uint32_t JPIT4 = JERRY + 0x0006;
inline constexpr uint32_t JPIT1_READ = JERRY + 0x0036;
inline constexpr uint32_t JPIT2_READ = JERRY + 0x0038;
inline constexpr uint32_t J_INT = JERRY + 0x0020;
inline constexpr uint32_t JOYSTICK = JERRY + 0x4000;
inline constexpr uint32_t JOYBUTS = JERRY + 0x4002;
inline constexpr uint32_t CONFIG = JERRY + 0x4002;
inline constexpr uint32_t GPIO0 = JERRY + 0x4800;
inline constexpr uint32_t GPIO1 = JERRY + 0x5000;

inline constexpr uint32_t DSP_FLAGS = JERRY + 0xA100;
inline constexpr uint32_t DSP_END = JERRY + 0xA10C;
inline constexpr uint32_t DSP_PC = JERRY + 0xA110;
inline constexpr uint32_t DSP_CTRL = JERRY + 0xA114;
inline constexpr uint32_t DSP_DIVCTRL = JERRY + 0xA11C;
inline constexpr uint32_t DSP_RIGHT = JERRY + 0xA148;
inline constexpr uint32_t DSP_LEFT = JERRY + 0xA14C;
inline constexpr uint32_t SCLK = JERRY + 0xA150;
inline constexpr uint32_t SMODE = JERRY + 0xA154;
inline constexpr uint32_t DSP_RAM = JERRY + 0xB000;
inline constexpr uint32_t DSP_RAM_SIZE = 0x2000;

inline constexpr uint16_t JOYSTICK_AUDIO_ON = 0x0100;
inline constexpr uint16_t JOYSTICK_OUTPUTS = 0x8000;

inline constexpr int SYSTEM_CLOCK_PAL = 26593900;
inline constexpr int SYSTEM_CLOCK_NTSC = 26590906;

template <typename T> inline volatile T &port(uint32_t address) {
  return *reinterpret_cast<volatile T *>(address);
}

inline volatile uint16_t &word(uint32_t address) {
  return port<uint16_t>(address);
}

inline volatile uint32_t &longWord(uint32_t address) {
  return port<uint32_t>(address);
}

} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_HARDWARE_H_
