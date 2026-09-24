#ifndef ENGINE_AMAL_PROGRAM_H_
#define ENGINE_AMAL_PROGRAM_H_

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace amal {

constexpr int16_t REGISTER_GLOBAL_BASE = 10;
constexpr int16_t REGISTER_X = 36;
constexpr int16_t REGISTER_Y = 37;
constexpr int16_t REGISTER_A = 38;

enum class TermKind : uint8_t { Number, Register, Joystick, Operator };

struct Term {
  TermKind kind = TermKind::Number;
  int16_t value = 0;
  char op = 0;
};

using Expression = std::vector<Term>;

enum class Opcode : uint8_t {
  Let,
  Move,
  Anim,
  Jump,
  IfJump,
  For,
  Next,
  Pause,
  Wait,
  End
};

struct AnimFrame {
  Expression image;
  Expression delay;
};

struct Instruction {
  Opcode opcode = Opcode::Pause;
  int16_t reg = 0;
  Expression first;
  Expression second;
  Expression third;
  int jump = -1;
  std::vector<AnimFrame> frames;
};

struct Program {
  std::vector<Instruction> code;
  std::array<int, 26> labels{};
};

Program parse(const std::string &source);

} // namespace amal
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_AMAL_PROGRAM_H_
