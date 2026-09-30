#ifndef ENGINE_AMAL_PROGRAM_H_
#define ENGINE_AMAL_PROGRAM_H_

#include <cstdint>
#include <string>
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
  Operator op = Operator::Add;
  int16_t value = 0;
};

struct Expression {
  uint16_t first = 0;
  uint16_t terms = 0;
};

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
  int16_t jump = -1;
  uint16_t firstFrame = 0;
  uint16_t frames = 0;
};

struct Program {
  const uint16_t *code = nullptr;
  const Instruction *instructions = nullptr;
  const Term *terms = nullptr;
  const AnimFrame *frames = nullptr;
  int16_t length = 0;
};

struct ParsedProgram {
  std::vector<uint16_t> code;
  std::vector<Instruction> instructions;
  std::vector<Term> terms;
  std::vector<AnimFrame> frames;

  Program program() const;
};

ParsedProgram parse(const std::string &source);

} // namespace amal
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_AMAL_PROGRAM_H_
