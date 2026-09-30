#include "Machine.h"

#include <cstdlib>
#include <stdexcept>

namespace openfranko::src::engine::amal {
namespace {

constexpr std::size_t SOURCE_HASH_SEED = 5381;
constexpr int SOURCE_HASH_SHIFT = 5;

int16_t toWord(int32_t value) { return static_cast<int16_t>(value); }

bool isChannel(int number) { return number >= 0 && number < Machine::CHANNELS; }

std::size_t slot(int number) {
  if (!isChannel(number)) {
    throw std::out_of_range("AMAL: no such channel");
  }
  return static_cast<std::size_t>(number);
}

int32_t moveStep(int16_t distance, int16_t frames) {
  const int32_t quotient = (std::abs(static_cast<int32_t>(distance)) << 8) /
                           static_cast<int32_t>(frames);
  if (quotient > 0xFFFF) {
    return 0;
  }
  const int16_t word = toWord(distance < 0 ? -quotient : quotient);
  return static_cast<int32_t>(word) * 256;
}

int16_t apply(Operator op, int16_t accumulator, int16_t value) {
  switch (op) {
  case Operator::Add:
    return toWord(accumulator + value);
  case Operator::Subtract:
    return toWord(accumulator - value);
  case Operator::Multiply:
    return toWord(accumulator * value);
  case Operator::Divide:
    return value != 0 ? toWord(accumulator / value) : accumulator;
  case Operator::Equal:
    return accumulator == value ? -1 : 0;
  case Operator::Less:
    return accumulator < value ? -1 : 0;
  case Operator::Greater:
    return accumulator > value ? -1 : 0;
  case Operator::NotEqual:
    return accumulator != value ? -1 : 0;
  case Operator::And:
    return toWord(accumulator & value);
  case Operator::Or:
    return toWord(accumulator | value);
  case Operator::Xor:
    return toWord(accumulator ^ value);
  }
  return accumulator;
}

} // namespace

std::size_t Machine::SourceHash::operator()(const std::string &source) const {
  std::size_t hash = SOURCE_HASH_SEED;
  for (const char letter : source) {
    hash =
        (hash << SOURCE_HASH_SHIFT) + hash + static_cast<unsigned char>(letter);
  }
  return hash;
}

Machine::Machine(Registers &globals) : m_globals(globals) {}

void Machine::bind(int channel, Object *object) {
  const std::size_t index = slot(channel);
  m_bindings[index] = object;
  if (m_channels[index].open) {
    m_channels[index].object = object;
  }
}

void Machine::create(int channel, const std::string &source) {
  const std::size_t index = slot(channel);
  std::shared_ptr<const Program> &program = m_programs[source];
  if (!program) {
    program = std::make_shared<const Program>(parse(source));
  }
  Channel created;
  created.program = program;
  created.open = true;
  created.instructions = static_cast<int>(program->code.size());
  created.loopLimits.assign(program->code.size(), 0);
  created.object = m_bindings[index];
  m_channels[index] = std::move(created);
}

void Machine::start(int channel) {
  if (Channel *started = opened(channel)) {
    started->frozen = false;
  }
}

void Machine::startAll() {
  for (Channel &each : m_channels) {
    each.frozen = !each.open;
  }
}

void Machine::freeze(int channel) {
  if (Channel *frozen = opened(channel)) {
    frozen->frozen = true;
  }
}

void Machine::freezeAll() {
  for (Channel &each : m_channels) {
    each.frozen = true;
  }
}

void Machine::destroy(int channel) {
  if (isChannel(channel)) {
    m_channels[static_cast<std::size_t>(channel)] = Channel{};
  }
}

void Machine::destroyAll() { m_channels.fill(Channel{}); }

bool Machine::exists(int channel) const { return opened(channel) != nullptr; }

bool Machine::isFrozen(int channel) const {
  const Channel *found = opened(channel);
  return found && found->frozen;
}

bool Machine::isRunning(int channel) const {
  const Channel *found = opened(channel);
  return found && !found->frozen && found->alive;
}

int16_t &Machine::channelRegister(int number, int index) {
  if (index < 0 || index > 9) {
    throw std::out_of_range("AMAL: no such channel register");
  }
  return channel(number).registers[static_cast<std::size_t>(index)];
}

int16_t &Machine::globalRegister(int index) {
  return m_globals.at(static_cast<std::size_t>(index));
}

void Machine::setJoystick(int16_t joystick) { m_joystick = joystick; }

void Machine::tick() {
  for (Channel &current : m_channels) {
    if (current.frozen) {
      continue;
    }
    run(current);
    stepAnim(current);
  }
}

Machine::Channel &Machine::channel(int number) {
  Channel *found = opened(number);
  if (!found) {
    throw std::out_of_range("AMAL: channel not opened");
  }
  return *found;
}

Machine::Channel *Machine::opened(int number) {
  if (!isChannel(number) ||
      !m_channels[static_cast<std::size_t>(number)].open) {
    return nullptr;
  }
  return &m_channels[static_cast<std::size_t>(number)];
}

const Machine::Channel *Machine::opened(int number) const {
  if (!isChannel(number) ||
      !m_channels[static_cast<std::size_t>(number)].open) {
    return nullptr;
  }
  return &m_channels[static_cast<std::size_t>(number)];
}

int16_t Machine::read(const Channel &channel, int16_t reg) const {
  if (reg < REGISTER_GLOBAL_BASE) {
    return channel.registers[static_cast<std::size_t>(reg)];
  }
  if (reg < REGISTER_X) {
    return m_globals[static_cast<std::size_t>(reg - REGISTER_GLOBAL_BASE)];
  }
  if (!channel.object) {
    return 0;
  }
  if (reg == REGISTER_X) {
    return channel.object->x;
  }
  return reg == REGISTER_Y ? channel.object->y : channel.object->image;
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

int16_t Machine::operand(const Channel &channel, const Term &term) const {
  switch (term.kind) {
  case TermKind::Register:
    return read(channel, term.value);
  case TermKind::Joystick:
    return m_joystick;
  case TermKind::Number:
  case TermKind::Operator:
    break;
  }
  return term.value;
}

int16_t Machine::evaluate(const Channel &channel,
                          const Expression &expression) const {
  auto term = expression.begin();
  const auto end = expression.end();
  if (term == end) {
    return 0;
  }
  int16_t accumulator = operand(channel, *term);
  while (++term != end) {
    const Operator op = term->op;
    if (++term == end) {
      break;
    }
    accumulator = apply(op, accumulator, operand(channel, *term));
  }
  return accumulator;
}

void Machine::run(Channel &channel) {
  if (!channel.alive) {
    return;
  }
  int jumps = 0;
  const auto &code = channel.program->code;
  for (;;) {
    if (channel.moveFrames > 0) {
      stepMove(channel);
      return;
    }
    if (channel.pc < 0 || channel.pc >= channel.instructions) {
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
      channel.program->code[static_cast<std::size_t>(channel.pc)];
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
      channel.program->code[static_cast<std::size_t>(channel.animInstruction)]
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
