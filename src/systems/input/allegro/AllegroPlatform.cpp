#include "input/Platform.h"

#include <algorithm>
#include <allegro.h>
#include <array>
#include <conio.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <go32.h>
#include <pc.h>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <sys/farptr.h>

namespace openfranko::src::systems::input {
namespace {

constexpr int SCANCODE_SHIFT = 8;
constexpr int CHARACTER_MASK = 0xFF;
constexpr int CONTROL_C = 3;
constexpr int LEFT_BUTTON = 1;
constexpr int FIRST_TYPED = ' ';
constexpr int LAST_TYPED = '~';
constexpr auto TIME_ZONE = "UTC0";
constexpr int CMOS_INDEX_PORT = 0x70;
constexpr int CMOS_DATA_PORT = 0x71;
constexpr int CMOS_EXTENDED_LOW = 0x30;
constexpr int CMOS_EXTENDED_HIGH = 0x31;
constexpr int BYTE_BITS = 8;
constexpr int KILOBYTES_PER_MEGABYTE = 1024;
constexpr int RESERVED_KILOBYTES = 512;
constexpr int NEEDED_MEGABYTES = 8;
constexpr unsigned long BIOS_TICKS_ADDRESS = 0x46C;
constexpr unsigned long WINDOW_TICKS = 2;
constexpr int SPEED_WINDOWS = 24;
constexpr int CHUNK_LOOPS = 1000;
constexpr long NEEDED_LOOPS = 1100000;
constexpr long DX33_LOOPS = 550000;
constexpr int ESCAPE = 27;
constexpr auto LOADING_MESSAGE = "Loading OpenFranko, please wait...\n";

std::array<bool, KEY_MAX> heldKeys{};

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Platform error: " + cause);
}

int memoryMegabytes() {
  outportb(CMOS_INDEX_PORT, CMOS_EXTENDED_LOW);
  const int low = inportb(CMOS_DATA_PORT);
  outportb(CMOS_INDEX_PORT, CMOS_EXTENDED_HIGH);
  const int high = inportb(CMOS_DATA_PORT);
  const int kilobytes = KILOBYTES_PER_MEGABYTE + (high << BYTE_BITS | low);
  return (kilobytes + RESERVED_KILOBYTES) / KILOBYTES_PER_MEGABYTE;
}

unsigned long biosTicks() { return _farpeekl(_dos_ds, BIOS_TICKS_ADDRESS); }

void spin(int loops) {
  asm volatile("1:\n\tdecl %0\n\tjnz 1b" : "+r"(loops) : : "cc");
}

long loopsPerWindow() {
  const unsigned long previous = biosTicks();
  unsigned long start = previous;
  while (start == previous) {
    start = biosTicks();
  }
  long loops = 0;
  while (biosTicks() - start < WINDOW_TICKS) {
    spin(CHUNK_LOOPS);
    loops += CHUNK_LOOPS;
  }
  return loops;
}

long cpuSpeed() {
  long best = 0;
  for (int window = 0; window < SPEED_WINDOWS && best < NEEDED_LOOPS;
       ++window) {
    best = std::max(best, loopsPerWindow());
  }
  return best;
}

const char *speedName(long loops) {
  if (loops >= NEEDED_LOOPS) {
    return "fast enough";
  }
  return loops >= DX33_LOOPS ? "about as fast as a 486DX-33"
                             : "about as fast as a 386";
}

bool startsAnyway() {
  while (true) {
    const int key = getch();
    if (key == 'y' || key == 'Y' || key == 'n' || key == 'N' || key == ESCAPE) {
      std::fputc('\n', stderr);
      return key == 'y' || key == 'Y';
    }
  }
}

bool isMachineAccepted() {
  const int memory = memoryMegabytes();
  const long speed = cpuSpeed();
  const bool fast = speed >= NEEDED_LOOPS;
  if (fast && memory >= NEEDED_MEGABYTES) {
    return true;
  }
  std::fprintf(stderr,
               "OpenFranko needs a 486DX2-66 or faster and %d MB of memory.\n"
               "This PC: %s, %d MB of memory.\n"
               "%s\n"
               "Start anyway? (Y/N)",
               NEEDED_MEGABYTES, speedName(speed), memory,
               fast ? "The music may stutter while the game loads."
                    : "The game will run in slow motion and the music may "
                      "stutter.");
  return startsAnyway();
}

Key toKey(int scancode) {
  switch (scancode) {
  case KEY_UP:
    return Key::Up;
  case KEY_DOWN:
    return Key::Down;
  case KEY_LEFT:
    return Key::Left;
  case KEY_RIGHT:
    return Key::Right;
  case KEY_SPACE:
    return Key::Space;
  case KEY_W:
    return Key::W;
  case KEY_A:
    return Key::A;
  case KEY_S:
    return Key::S;
  case KEY_D:
    return Key::D;
  case KEY_DEL:
    return Key::Delete;
  case KEY_F1:
    return Key::F1;
  case KEY_F2:
    return Key::F2;
  case KEY_F3:
    return Key::F3;
  case KEY_F4:
    return Key::F4;
  case KEY_F9:
    return Key::F9;
  case KEY_ESC:
    return Key::Escape;
  default:
    return Key::Other;
  }
}

char typedCharacter(int scancode) {
  if (scancode == KEY_BACKSPACE) {
    return '\b';
  }
  const int character = scancode_to_ascii(scancode);
  if (character == '\r' ||
      (character >= FIRST_TYPED && character <= LAST_TYPED)) {
    return static_cast<char>(character);
  }
  return 0;
}

bool isBreak(int typed) {
  return (typed & CHARACTER_MASK) == CONTROL_C ||
         ((typed >> SCANCODE_SHIFT) == KEY_PAUSE &&
          (key_shifts & KB_CTRL_FLAG) != 0);
}

void receiveKey(ControllerSystem &controller, int scancode, bool pressed) {
  bool &held = heldKeys[static_cast<std::size_t>(scancode)];
  KeyEvent event;
  event.key = toKey(scancode);
  event.code = scancode;
  event.character = typedCharacter(scancode);
  event.pressed = pressed;
  event.repeat = pressed && held;
  held = pressed;
  controller.receiveKey(event);
}

} // namespace

Platform::Platform() {
  if (!isMachineAccepted()) {
    std::exit(EXIT_SUCCESS);
  }
  std::fputs(LOADING_MESSAGE, stderr);
  setenv("TZ", TIME_ZONE, 0);
  if (allegro_init() != 0 || install_timer() != 0 || install_keyboard() != 0) {
    throwError(allegro_error);
  }
  three_finger_flag = FALSE;
  install_mouse();
}

Platform::~Platform() { allegro_exit(); }

bool Platform::pollEvents(ControllerSystem &controller) {
  bool open = true;
  while (keypressed()) {
    const int typed = readkey();
    if (isBreak(typed)) {
      open = false;
    }
    receiveKey(controller, typed >> SCANCODE_SHIFT, true);
  }
  for (int scancode = 0; scancode < KEY_MAX; ++scancode) {
    const bool held = key[scancode] != 0;
    if (held != heldKeys[static_cast<std::size_t>(scancode)]) {
      receiveKey(controller, scancode, held);
    }
  }
  controller.receiveMouseButton((mouse_b & LEFT_BUTTON) != 0);
  return open;
}

} // namespace openfranko::src::systems::input
