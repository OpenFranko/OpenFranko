#ifndef ENGINE_AMAL_MACHINE_H_
#define ENGINE_AMAL_MACHINE_H_

#include "Program.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace amal {

using Registers = std::array<int16_t, 26>;

struct Object {
  int16_t x = 0;
  int16_t y = 0;
  int16_t image = 0;
};

class Machine {
public:
  static constexpr int JUMP_BUDGET = 10;

  explicit Machine(Registers &globals);

  void bind(int channel, Object *object);
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

  int16_t &channelRegister(int channel, int index);
  int16_t &globalRegister(int index);

  void setJoystick(int16_t joystick);
  void tick();

private:
  struct Channel {
    Program program;
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

  Channel &channel(int number);
  int16_t read(const Channel &channel, int16_t reg) const;
  void write(Channel &channel, int16_t reg, int16_t value);
  int16_t evaluate(const Channel &channel, const Expression &expression) const;
  void run(Channel &channel);
  void startMove(Channel &channel, int16_t dx, int16_t dy, int16_t frames);
  void stepMove(Channel &channel);
  void startAnim(Channel &channel);
  void stepAnim(Channel &channel);

  Registers &m_globals;
  std::map<int, Object *> m_bindings;
  std::map<int, Channel> m_channels;
  int16_t m_joystick = 0;
};

} // namespace amal
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_AMAL_MACHINE_H_
