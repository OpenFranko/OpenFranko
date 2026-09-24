#include "Program.h"

#include <cctype>
#include <stdexcept>

namespace openfranko::src::engine::amal {
namespace {

bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)); }

bool isHexDigit(char c) { return std::isxdigit(static_cast<unsigned char>(c)); }

bool isUpper(char c) { return std::isupper(static_cast<unsigned char>(c)); }

std::string tokenize(const std::string &source) {
  std::string tokens;
  for (char c : source) {
    if (!std::islower(static_cast<unsigned char>(c)) &&
        !std::isspace(static_cast<unsigned char>(c))) {
      tokens += c;
    }
  }
  return tokens;
}

bool isOperator(char c) {
  switch (c) {
  case '+':
  case '-':
  case '*':
  case '/':
  case '=':
  case '<':
  case '>':
  case '&':
  case '|':
  case '!':
    return true;
  default:
    return false;
  }
}

class Parser {
public:
  explicit Parser(std::string tokens) : m_tokens(std::move(tokens)) {}

  Program parse() {
    Program program;
    program.labels.fill(-1);
    std::vector<std::pair<std::size_t, char>> jumps;
    std::vector<std::size_t> openLoops;

    while (!atEnd()) {
      const char c = peek();
      if (c == ';') {
        ++m_position;
        continue;
      }
      if (isUpper(c) && peek(1) == ':') {
        program.labels[labelIndex(c)] = static_cast<int>(program.code.size());
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
        while (peek() == '(') {
          ++m_position;
          AnimFrame frame;
          frame.image = expression(",");
          expect(',');
          frame.delay = expression(")");
          expect(')');
          instruction.frames.push_back(std::move(frame));
        }
        break;
      case 'I':
        instruction.opcode = Opcode::IfJump;
        instruction.first = expression("J");
        expect('J');
        jumps.emplace_back(program.code.size(), next());
        break;
      case 'J':
        instruction.opcode = Opcode::Jump;
        jumps.emplace_back(program.code.size(), next());
        break;
      case 'F':
        instruction.opcode = Opcode::For;
        instruction.reg = reg();
        expect('=');
        instruction.first = expression("T");
        expect('T');
        instruction.second = expression(";");
        openLoops.push_back(program.code.size());
        break;
      case 'N':
        instruction.opcode = Opcode::Next;
        instruction.reg = reg();
        if (openLoops.empty() ||
            program.code[openLoops.back()].reg != instruction.reg) {
          throw std::invalid_argument("AMAL: Next without For");
        }
        instruction.jump = static_cast<int>(openLoops.back());
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
      program.code.push_back(std::move(instruction));
    }

    for (const auto &[index, label] : jumps) {
      const int target = program.labels[labelIndex(label)];
      if (target < 0) {
        throw std::invalid_argument(std::string("AMAL: undefined label ") +
                                    label);
      }
      program.code[index].jump = target;
    }
    return program;
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

  Expression expression(const std::string &stops) {
    Expression expression;
    while (!atEnd()) {
      const char c = peek();
      if (stops.find(c) != std::string::npos && !isJoystick()) {
        break;
      }
      Term term;
      if (isOperator(c)) {
        term.kind = TermKind::Operator;
        if (c == '<' && peek(1) == '>') {
          term.op = '#';
          m_position += 2;
        } else {
          term.op = c;
          ++m_position;
        }
      } else if (c == '$') {
        ++m_position;
        uint32_t value = 0;
        while (!atEnd() && isHexDigit(peek())) {
          const char digit = next();
          value = value * 16 + static_cast<uint32_t>(isDigit(digit)
                                                         ? digit - '0'
                                                         : digit - 'A' + 10);
        }
        term.value = static_cast<int16_t>(value);
      } else if (isDigit(c)) {
        uint32_t value = 0;
        while (!atEnd() && isDigit(peek())) {
          value = value * 10 + static_cast<uint32_t>(next() - '0');
        }
        term.value = static_cast<int16_t>(value);
      } else if (isJoystick()) {
        m_position += 2;
        term.kind = TermKind::Joystick;
      } else {
        term.kind = TermKind::Register;
        term.value = reg();
      }
      expression.push_back(term);
    }
    return expression;
  }

  std::string m_tokens;
  std::size_t m_position = 0;
};

} // namespace

Program parse(const std::string &source) {
  return Parser(tokenize(source)).parse();
}

} // namespace openfranko::src::engine::amal
