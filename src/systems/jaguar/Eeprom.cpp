#include "Eeprom.h"

#include "Hardware.h"

namespace openfranko::src::systems::jaguar::eeprom {
namespace {

constexpr uint16_t READ_COMMAND = 0x180;
constexpr uint16_t ENABLE_COMMAND = 0x130;
constexpr uint16_t WRITE_COMMAND = 0x140;
constexpr uint16_t DISABLE_COMMAND = 0x100;
constexpr uint16_t ADDRESS_MASK = 0x3F;
constexpr int COMMAND_BITS = 9;
constexpr int DATA_BITS = 16;
constexpr int CHECKSUM_ADDRESS = DATA_WORDS;
constexpr int NOTHING_QUEUED = -1;
constexpr int ATTEMPTS = 3;

std::array<uint16_t, WORDS> queued{};
int nextWord = NOTHING_QUEUED;
int writing = NOTHING_QUEUED;
int attempts = 0;

void pause() { asm volatile("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop"); }

void strobe(uint32_t address) {
  [[maybe_unused]] const uint16_t value = word(address);
}

void select() {
  strobe(GPIO1);
  pause();
}

void sendBits(uint16_t value, int bits) {
  for (int bit = bits - 1; bit >= 0; --bit) {
    word(GPIO0) = static_cast<uint16_t>((value >> bit) & 1);
    pause();
  }
}

uint16_t readWord(int address) {
  select();
  sendBits(static_cast<uint16_t>(READ_COMMAND | (address & ADDRESS_MASK)),
           COMMAND_BITS);
  uint16_t value = 0;
  for (int bit = 0; bit < DATA_BITS; ++bit) {
    strobe(GPIO0);
    pause();
    value = static_cast<uint16_t>(value << 1 | (word(JOYSTICK) & 1));
    pause();
  }
  return value;
}

void startWrite(int address, uint16_t value) {
  select();
  sendBits(ENABLE_COMMAND, COMMAND_BITS);
  select();
  sendBits(static_cast<uint16_t>(WRITE_COMMAND | (address & ADDRESS_MASK)),
           COMMAND_BITS);
  sendBits(value, DATA_BITS);
  select();
}

void finishWrite() {
  sendBits(DISABLE_COMMAND, COMMAND_BITS);
  select();
}

} // namespace

uint16_t checksum(const Bank &bank) {
  uint16_t sum = 0;
  for (const uint16_t value : bank) {
    sum = static_cast<uint16_t>(sum + value);
  }
  return static_cast<uint16_t>(sum ^ 0xFFFF);
}

bool readBank(Bank &bank) {
  for (int address = 0; address < DATA_WORDS; ++address) {
    bank[static_cast<std::size_t>(address)] = readWord(address);
  }
  return readWord(CHECKSUM_ADDRESS) == checksum(bank);
}

void queueBank(const Bank &bank) {
  for (int address = 0; address < DATA_WORDS; ++address) {
    queued[static_cast<std::size_t>(address)] =
        bank[static_cast<std::size_t>(address)];
  }
  queued[CHECKSUM_ADDRESS] = checksum(bank);
  nextWord = 0;
}

bool isWriting() {
  return nextWord != NOTHING_QUEUED || writing != NOTHING_QUEUED;
}

void poll() {
  if (writing != NOTHING_QUEUED) {
    finishWrite();
    const int address = writing;
    writing = NOTHING_QUEUED;
    if (readWord(address) != queued[static_cast<std::size_t>(address)] &&
        ++attempts < ATTEMPTS) {
      startWrite(address, queued[static_cast<std::size_t>(address)]);
      writing = address;
      return;
    }
  }
  while (nextWord != NOTHING_QUEUED) {
    const int address = nextWord;
    nextWord = address + 1 < WORDS ? address + 1 : NOTHING_QUEUED;
    const uint16_t value = queued[static_cast<std::size_t>(address)];
    if (readWord(address) != value) {
      startWrite(address, value);
      writing = address;
      attempts = 0;
      return;
    }
  }
}

} // namespace openfranko::src::systems::jaguar::eeprom
