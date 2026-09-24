#include "Machine.h"

#include <cstdlib>
#include <stdexcept>

namespace openfranko::src::engine::amal {
namespace {

int16_t toWord(int32_t value) { return static_cast<int16_t>(value); }

int32_t moveStep(int16_t distance, int16_t frames) {
  const int32_t quotient = (std::abs(static_cast<int32_t>(distance)) << 8) /
                           static_cast<int32_t>(frames);
  if (quotient > 0xFFFF) {
    return 0;
  }
  const int16_t word = toWord(distance < 0 ? -quotient : quotient);
  return static_cast<int32_t>(word) * 256;
}

} // namespace

Machine::Machine(Registers &globals) : m_globals(globals) {}

void Machine::bind(int channel, Object *object) {
  m_bindings[channel] = object;
  auto it = m_channels.find(channel);
  if (it != m_channels.end()) {
    it->second.object = object;
  }
}

void Machine::create(int channel, const std::string &source) {
  Channel created;
  created.program = parse(source);
  created.loopLimits.assign(created.program.code.size(), 0);
  auto binding = m_bindings.find(channel);
  created.object = binding != m_bindings.end() ? binding->second : nullptr;
  m_channels[channel] = std::move(created);
}

void Machine::start(int channel) {
  auto it = m_channels.find(channel);
  if (it != m_channels.end()) {
    it->second.frozen = false;
  }
}

void Machine::startAll() {
  for (auto &entry : m_channels) {
    entry.second.frozen = false;
  }
}

void Machine::freeze(int channel) {
  auto it = m_channels.find(channel);
  if (it != m_channels.end()) {
    it->second.frozen = true;
  }
}

void Machine::freezeAll() {
  for (auto &entry : m_channels) {
    entry.second.frozen = true;
  }
}

void Machine::destroy(int channel) { m_channels.erase(channel); }

void Machine::destroyAll() { m_channels.clear(); }

bool Machine::exists(int channel) const {
  return m_channels.find(channel) != m_channels.end();
}

bool Machine::isFrozen(int channel) const {
  auto it = m_channels.find(channel);
  return it != m_channels.end() && it->second.frozen;
}

bool Machine::isRunning(int channel) const {
  auto it = m_channels.find(channel);
  return it != m_channels.end() && !it->second.frozen && it->second.alive;
}

int16_t &Machine::channelRegister(int channel, int index) {
  if (index < 0 || index > 9) {
    throw std::out_of_range("AMAL: no such channel register");
  }
  return this->channel(channel).registers[static_cast<std::size_t>(index)];
}

int16_t &Machine::globalRegister(int index) {
  return m_globals.at(static_cast<std::size_t>(index));
}

void Machine::setJoystick(int16_t joystick) { m_joystick = joystick; }

void Machine::tick() {
  for (auto &entry : m_channels) {
    Channel &current = entry.second;
    if (current.frozen) {
      continue;
    }
    run(current);
    stepAnim(current);
  }
}

Machine::Channel &Machine::channel(int number) {
  auto it = m_channels.find(number);
  if (it == m_channels.end()) {
    throw std::out_of_range("AMAL: channel not opened");
  }
  return it->second;
}

int16_t Machine::read(const Channel &channel, int16_t reg) const {
  switch (reg) {
  case REGISTER_X:
    return channel.object ? channel.object->x : 0;
  case REGISTER_Y:
    return channel.object ? channel.object->y : 0;
  case REGISTER_A:
    return channel.object ? channel.object->image : 0;
  default:
    if (reg < REGISTER_GLOBAL_BASE) {
      return channel.registers[static_cast<std::size_t>(reg)];
    }
    return m_globals[static_cast<std::size_t>(reg - REGISTER_GLOBAL_BASE)];
  }
}

void Machine::write(Channel &channel, int16_t reg, int16_t value) {
  switch (reg) {
  case REGISTER_X:
    if (channel.object) {
      channel.object->x = value;
    }
    break;
  case REGISTER_Y:
    if (channel.object) {
      channel.object->y = value;
    }
    break;
  case REGISTER_A:
    if (channel.object) {
      channel.object->image = value;
    }
    break;
  default:
    if (reg < REGISTER_GLOBAL_BASE) {
      channel.registers[static_cast<std::size_t>(reg)] = value;
    } else {
      m_globals[static_cast<std::size_t>(reg - REGISTER_GLOBAL_BASE)] = value;
    }
    break;
  }
}

int16_t Machine::evaluate(const Channel &channel,
                          const Expression &expression) const {
  int16_t accumulator = 0;
  char pending = 0;
  for (const Term &term : expression) {
    if (term.kind == TermKind::Operator) {
      pending = term.op;
      continue;
    }
    int16_t value = term.value;
    if (term.kind == TermKind::Joystick) {
      value = m_joystick;
    } else if (term.kind == TermKind::Register) {
      value = read(channel, term.value);
    }
    switch (pending) {
    case '+':
      accumulator = toWord(accumulator + value);
      break;
    case '-':
      accumulator = toWord(accumulator - value);
      break;
    case '*':
      accumulator = toWord(accumulator * value);
      break;
    case '/':
      if (value != 0) {
        accumulator = toWord(accumulator / value);
      }
      break;
    case '=':
      accumulator = accumulator == value ? -1 : 0;
      break;
    case '<':
      accumulator = accumulator < value ? -1 : 0;
      break;
    case '>':
      accumulator = accumulator > value ? -1 : 0;
      break;
    case '#':
      accumulator = accumulator != value ? -1 : 0;
      break;
    case '&':
      accumulator = toWord(accumulator & value);
      break;
    case '|':
      accumulator = toWord(accumulator | value);
      break;
    case '!':
      accumulator = toWord(accumulator ^ value);
      break;
    default:
      accumulator = value;
      break;
    }
    pending = 0;
  }
  return accumulator;
}

void Machine::run(Channel &channel) {
  if (!channel.alive) {
    return;
  }
  int jumps = 0;
  const auto &code = channel.program.code;
  for (;;) {
    if (channel.moveFrames > 0) {
      stepMove(channel);
      return;
    }
    if (channel.pc < 0 || channel.pc >= static_cast<int>(code.size())) {
      channel.alive = false;
      return;
    }
    const Instruction &instruction = code[static_cast<std::size_t>(channel.pc)];
    switch (instruction.opcode) {
    case Opcode::Pause:
      ++channel.pc;
      return;
    case Opcode::Wait:
    case Opcode::End:
      ++channel.pc;
      channel.alive = false;
      return;
    case Opcode::Let:
      write(channel, instruction.reg, evaluate(channel, instruction.first));
      ++channel.pc;
      break;
    case Opcode::Move: {
      const int16_t dx = evaluate(channel, instruction.first);
      const int16_t dy = evaluate(channel, instruction.second);
      const int16_t frames = evaluate(channel, instruction.third);
      ++channel.pc;
      startMove(channel, dx, dy, frames);
      stepMove(channel);
      return;
    }
    case Opcode::Anim:
      startAnim(channel);
      ++channel.pc;
      break;
    case Opcode::Jump:
      channel.pc = instruction.jump;
      if (++jumps >= JUMP_BUDGET) {
        return;
      }
      break;
    case Opcode::IfJump:
      if (evaluate(channel, instruction.first) != 0) {
        channel.pc = instruction.jump;
        if (++jumps >= JUMP_BUDGET) {
          return;
        }
      } else {
        ++channel.pc;
      }
      break;
    case Opcode::For: {
      const int16_t start = evaluate(channel, instruction.first);
      channel.loopLimits[static_cast<std::size_t>(channel.pc)] =
          evaluate(channel, instruction.second);
      write(channel, instruction.reg, start);
      ++channel.pc;
      break;
    }
    case Opcode::Next: {
      const Instruction &loop =
          code[static_cast<std::size_t>(instruction.jump)];
      const int16_t value = toWord(read(channel, loop.reg) + 1);
      write(channel, loop.reg, value);
      if (value <=
          channel.loopLimits[static_cast<std::size_t>(instruction.jump)]) {
        channel.pc = instruction.jump + 1;
        return;
      }
      ++channel.pc;
      break;
    }
    }
  }
}

void Machine::startMove(Channel &channel, int16_t dx, int16_t dy,
                        int16_t frames) {
  if (frames <= 0) {
    frames = 1;
  }
  channel.stepX = moveStep(dx, frames);
  channel.stepY = moveStep(dy, frames);
  channel.fractionX = 0x8000;
  channel.fractionY = 0x8000;
  channel.moveFrames = frames;
}

void Machine::stepMove(Channel &channel) {
  --channel.moveFrames;
  if (!channel.object) {
    return;
  }
  const uint32_t x =
      (static_cast<uint32_t>(static_cast<uint16_t>(channel.object->x)) << 16 |
       channel.fractionX) +
      static_cast<uint32_t>(channel.stepX);
  const uint32_t y =
      (static_cast<uint32_t>(static_cast<uint16_t>(channel.object->y)) << 16 |
       channel.fractionY) +
      static_cast<uint32_t>(channel.stepY);
  channel.object->x = static_cast<int16_t>(x >> 16);
  channel.object->y = static_cast<int16_t>(y >> 16);
  channel.fractionX = static_cast<uint16_t>(x & 0xFFFF);
  channel.fractionY = static_cast<uint16_t>(y & 0xFFFF);
}

void Machine::startAnim(Channel &channel) {
  const Instruction &instruction =
      channel.program.code[static_cast<std::size_t>(channel.pc)];
  channel.animInstruction = channel.pc;
  channel.animLoops = evaluate(channel, instruction.first);
  channel.animNext = 0;
  channel.animCounter = 1;
}

void Machine::stepAnim(Channel &channel) {
  if (channel.animInstruction < 0) {
    return;
  }
  const auto &frames =
      channel.program.code[static_cast<std::size_t>(channel.animInstruction)]
          .frames;
  if (frames.empty() || --channel.animCounter != 0) {
    return;
  }
  if (channel.animNext >= frames.size()) {
    if (channel.animLoops != 0) {
      channel.animLoops = toWord(channel.animLoops - 1);
      if (channel.animLoops == 0) {
        channel.animInstruction = -1;
        return;
      }
    }
    channel.animNext = 0;
  }
  const AnimFrame &frame = frames[channel.animNext++];
  write(channel, REGISTER_A, evaluate(channel, frame.image));
  channel.animCounter = static_cast<uint16_t>(evaluate(channel, frame.delay));
}

} // namespace openfranko::src::engine::amal
