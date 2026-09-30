#include "Program.h"

#include <array>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace openfranko::src::engine::amal {
namespace {

constexpr std::size_t LETTERS = 26;

bool isDigit(char c) { return c >= '0' && c <= '9'; }

bool isUpper(char c) { return c >= 'A' && c <= 'Z'; }

bool isLower(char c) { return c >= 'a' && c <= 'z'; }

bool isHexDigit(char c) {
  return isDigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

bool isSpace(char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }

std::string tokenize(const std::string &source) {
  std::string tokens;
  tokens.reserve(source.size());
  for (char c : source) {
    if (!isLower(c) && !isSpace(c)) {
      tokens += c;
    }
  }
  return tokens;
}

std::optional<Operator> toOperator(char c) {
  switch (c) {
  case '+':
    return Operator::Add;
  case '-':
    return Operator::Subtract;
  case '*':
    return Operator::Multiply;
  case '/':
    return Operator::Divide;
  case '=':
    return Operator::Equal;
  case '<':
    return Operator::Less;
  case '>':
    return Operator::Greater;
  case '&':
    return Operator::And;
  case '|':
    return Operator::Or;
  case '!':
    return Operator::Xor;
  default:
    return std::nullopt;
  }
}

class Parser {
public:
  explicit Parser(std::string tokens) : m_tokens(std::move(tokens)) {}

  ParsedProgram parse() {
    ParsedProgram &program = m_program;
    std::array<int, LETTERS> labels;
    labels.fill(-1);
    std::vector<std::pair<std::size_t, char>> jumps;
    std::vector<std::size_t> openLoops;

    while (!atEnd()) {
      const char c = peek();
      if (c == ';') {
        ++m_position;
        continue;
      }
      if (isUpper(c) && peek(1) == ':') {
        labels[labelIndex(c)] = static_cast<int>(program.instructions.size());
        m_position += 2;
        continue;
      }
      ++m_position;
      Instruction instruction;
      switch (c) {
      case 'L':
        instruction.opcode = Opcode::Let;
        instruction.reg = reg();
        expect('=');
        instruction.first = expression(";");
        break;
      case 'M':
        instruction.opcode = Opcode::Move;
        instruction.first = expression(",;");
        expect(',');
        instruction.second = expression(",;");
        expect(',');
        instruction.third = expression(";");
        break;
      case 'A':
        instruction.opcode = Opcode::Anim;
        instruction.first = expression(",;");
        if (peek() == ',') {
          ++m_position;
        }
        instruction.firstFrame = static_cast<uint16_t>(program.frames.size());
        while (peek() == '(') {
          ++m_position;
          AnimFrame frame;
          frame.image = expression(",");
          expect(',');
          frame.delay = expression(")");
          expect(')');
          program.frames.push_back(frame);
          ++instruction.frames;
        }
        break;
      case 'I':
        instruction.opcode = Opcode::IfJump;
        instruction.first = expression("J");
        expect('J');
        jumps.emplace_back(program.instructions.size(), next());
        break;
      case 'J':
        instruction.opcode = Opcode::Jump;
        jumps.emplace_back(program.instructions.size(), next());
        break;
      case 'F':
        instruction.opcode = Opcode::For;
        instruction.reg = reg();
        expect('=');
        instruction.first = expression("T");
        expect('T');
        instruction.second = expression(";");
        openLoops.push_back(program.instructions.size());
        break;
      case 'N':
        instruction.opcode = Opcode::Next;
        instruction.reg = reg();
        if (openLoops.empty() ||
            program.instructions[openLoops.back()].reg != instruction.reg) {
          throw std::invalid_argument("AMAL: Next without For");
        }
        instruction.jump = static_cast<int16_t>(openLoops.back());
        openLoops.pop_back();
        break;
      case 'P':
        instruction.opcode = Opcode::Pause;
        break;
      case 'W':
        instruction.opcode = Opcode::Wait;
        break;
      case 'E':
        instruction.opcode = Opcode::End;
        break;
      default:
        throw std::invalid_argument(std::string("AMAL: unknown instruction ") +
                                    c);
      }
      program.instructions.push_back(instruction);
    }

    for (const auto &[index, label] : jumps) {
      const int target = labels[labelIndex(label)];
      if (target < 0) {
        throw std::invalid_argument(std::string("AMAL: undefined label ") +
                                    label);
      }
      program.instructions[index].jump = static_cast<int16_t>(target);
    }
    program.code.resize(program.instructions.size());
    for (std::size_t i = 0; i < program.code.size(); ++i) {
      program.code[i] = static_cast<uint16_t>(i);
    }
    return std::move(program);
  }

private:
  bool atEnd() const { return m_position >= m_tokens.size(); }

  char peek(std::size_t ahead = 0) const {
    return m_position + ahead < m_tokens.size() ? m_tokens[m_position + ahead]
                                                : '\0';
  }

  char next() {
    if (atEnd()) {
      throw std::invalid_argument("AMAL: unexpected end of program");
    }
    return m_tokens[m_position++];
  }

  void expect(char c) {
    if (next() != c) {
      throw std::invalid_argument(std::string("AMAL: expected ") + c);
    }
  }

  static std::size_t labelIndex(char c) {
    if (!isUpper(c)) {
      throw std::invalid_argument(std::string("AMAL: bad label ") + c);
    }
    return static_cast<std::size_t>(c - 'A');
  }

  int16_t reg() {
    const char c = next();
    if (c == 'R') {
      const char r = next();
      if (isDigit(r)) {
        return static_cast<int16_t>(r - '0');
      }
      if (isUpper(r)) {
        return static_cast<int16_t>(REGISTER_GLOBAL_BASE + (r - 'A'));
      }
      throw std::invalid_argument(std::string("AMAL: bad register R") + r);
    }
    if (c == 'X') {
      return REGISTER_X;
    }
    if (c == 'Y') {
      return REGISTER_Y;
    }
    if (c == 'A') {
      return REGISTER_A;
    }
    throw std::invalid_argument(std::string("AMAL: expected a register, got ") +
                                c);
  }

  bool isJoystick() const { return peek() == 'J' && isDigit(peek(1)); }

  bool isNumber(std::size_t ahead) const {
    return isDigit(peek(ahead)) || peek(ahead) == '$';
  }

  int16_t number() {
    const bool negative = peek() == '-';
    if (negative) {
      ++m_position;
    }
    uint32_t value = 0;
    if (peek() == '$') {
      ++m_position;
      while (!atEnd() && isHexDigit(peek())) {
        const char digit = next();
        value = value * 16 + static_cast<uint32_t>(isDigit(digit)
                                                       ? digit - '0'
                                                       : digit - 'A' + 10);
      }
    } else {
      while (!atEnd() && isDigit(peek())) {
        value = value * 10 + static_cast<uint32_t>(next() - '0');
      }
    }
    return static_cast<int16_t>(negative ? 0u - value : value);
  }

  Term operand() {
    Term term;
    if (isNumber(0) || (peek() == '-' && isNumber(1))) {
      term.value = number();
    } else if (isJoystick()) {
      m_position += 2;
      term.kind = TermKind::Joystick;
    } else {
      term.kind = TermKind::Register;
      term.value = reg();
    }
    return term;
  }

  Term binaryOperator() {
    const char c = next();
    const std::optional<Operator> op = toOperator(c);
    if (!op) {
      throw std::invalid_argument(
          std::string("AMAL: expected an operator, got ") + c);
    }
    Term term;
    term.kind = TermKind::Operator;
    term.op = *op;
    if (*op == Operator::Less && peek() == '>') {
      term.op = Operator::NotEqual;
      ++m_position;
    }
    return term;
  }

  Expression expression(std::string_view stops) {
    std::vector<Term> &terms = m_program.terms;
    const std::size_t first = terms.size();
    terms.push_back(operand());
    while (!atEnd() && stops.find(peek()) == std::string_view::npos) {
      terms.push_back(binaryOperator());
      terms.push_back(operand());
    }
    return Expression{static_cast<uint16_t>(first),
                      static_cast<uint16_t>(terms.size() - first)};
  }

  std::string m_tokens;
  std::size_t m_position = 0;
  ParsedProgram m_program;
};

} // namespace

Program ParsedProgram::program() const {
  return Program{code.data(), instructions.data(), terms.data(), frames.data(),
                 static_cast<int16_t>(instructions.size())};
}

ParsedProgram parse(const std::string &source) {
  return Parser(tokenize(source)).parse();
}

} // namespace openfranko::src::engine::amal
