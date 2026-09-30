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

inline constexpr int16_t REGISTER_GLOBAL_BASE = 10;
inline constexpr int16_t REGISTER_X = 36;
inline constexpr int16_t REGISTER_Y = 37;
inline constexpr int16_t REGISTER_A = 38;

enum class TermKind : uint8_t { Number, Register, Joystick, Operator };

enum class Operator : uint8_t {
  Add,
  Subtract,
  Multiply,
  Divide,
  Equal,
  Less,
  Greater,
  NotEqual,
  And,
  Or,
  Xor
};

struct Term {
  TermKind kind = TermKind::Number;
  int16_t value = 0;
  Operator op = Operator::Add;
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
