#ifndef ENGINE_AMAL_MACHINE_H_
#define ENGINE_AMAL_MACHINE_H_

#include "Program.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace amal {

using Registers = std::array<int16_t, 26>;

inline constexpr int RA = 0;
inline constexpr int RB = 1;
inline constexpr int RC = 2;
inline constexpr int RD = 3;
inline constexpr int RE = 4;
inline constexpr int RF = 5;
inline constexpr int RG = 6;
inline constexpr int RH = 7;
inline constexpr int RI = 8;
inline constexpr int RJ = 9;
inline constexpr int RK = 10;
inline constexpr int RL = 11;
inline constexpr int RM = 12;
inline constexpr int RN = 13;
inline constexpr int RO = 14;
inline constexpr int RP = 15;
inline constexpr int RQ = 16;
inline constexpr int RR = 17;
inline constexpr int RS = 18;
inline constexpr int RT = 19;
inline constexpr int RU = 20;
inline constexpr int RV = 21;
inline constexpr int RW = 22;
inline constexpr int RX = 23;
inline constexpr int RY = 24;
inline constexpr int RZ = 25;

struct Object {
  int16_t x = 0;
  int16_t y = 0;
  int16_t image = 0;
};

class Machine {
public:
  static constexpr int JUMP_BUDGET = 10;
  static constexpr int CHANNELS = 64;

  explicit Machine(Registers &globals);

  void bind(int channel, Object *object);
  void create(int channel, const Program &program);
  void create(int channel, const std::string &source);
  void start(int channel);
  void startAll();
  void freeze(int channel);
  void freezeAll();
  void destroy(int channel);
  void destroyAll();

  bool exists(int channel) const;
  bool isFrozen(int channel) const;
  bool isRunning(int channel) const;

  int16_t &channelRegister(int number, int index);
  int16_t &globalRegister(int index);

  void setJoystick(int16_t joystick);
  void tick();

private:
  struct Channel {
    Program program;
    bool open = false;
    int pc = 0;
    bool alive = true;
    bool frozen = true;
    std::array<int16_t, 10> registers{};
    Object *object = nullptr;
    std::vector<int16_t> loopLimits;

    int32_t stepX = 0;
    int32_t stepY = 0;
    uint16_t fractionX = 0;
    uint16_t fractionY = 0;
    int moveFrames = 0;

    int animInstruction = -1;
    int16_t animLoops = 0;
    std::size_t animNext = 0;
    uint16_t animCounter = 0;
  };

  struct SourceHash {
    std::size_t operator()(const std::string &source) const;
  };

  Channel &channel(int number);
  Channel *opened(int number);
  const Channel *opened(int number) const;
  int16_t read(const Channel &channel, int16_t reg) const;
  void write(Channel &channel, int16_t reg, int16_t value);
  int16_t operand(const Channel &channel, const Term &term) const;
  int16_t evaluate(const Channel &channel, const Expression &expression) const;
  void run(Channel &channel);
  void startMove(Channel &channel, int16_t dx, int16_t dy, int16_t frames);
  void stepMove(Channel &channel);
  void startAnim(Channel &channel);
  void stepAnim(Channel &channel);

  Registers &m_globals;
  std::array<Object *, CHANNELS> m_bindings{};
  std::array<Channel, CHANNELS> m_channels{};
  std::unordered_map<std::string, ParsedProgram, SourceHash> m_programs;
  int16_t m_joystick = 0;
};

} // namespace amal
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_AMAL_MACHINE_H_
