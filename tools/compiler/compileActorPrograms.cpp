#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include "../../src/engine/amal/Program.h"
#include "../../src/engine/street/actors/Actors.h"
#include "../../src/engine/street/actors/compiled/CompiledActors.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace openfranko::lib;
using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street;

namespace {

constexpr std::size_t NUMBERS_PER_LINE = 12;

const char *termKindName(amal::TermKind kind) {
  switch (kind) {
  case amal::TermKind::Number:
    return "NUMBER";
  case amal::TermKind::Register:
    return "REGISTER";
  case amal::TermKind::Joystick:
    return "JOYSTICK";
  case amal::TermKind::Operator:
    break;
  }
  return "OPERATOR";
}

const char *operatorName(amal::Operator op) {
  switch (op) {
  case amal::Operator::Add:
    return "ADD";
  case amal::Operator::Subtract:
    return "SUBTRACT";
  case amal::Operator::Multiply:
    return "MULTIPLY";
  case amal::Operator::Divide:
    return "DIVIDE";
  case amal::Operator::Equal:
    return "EQUAL";
  case amal::Operator::Less:
    return "LESS";
  case amal::Operator::Greater:
    return "GREATER";
  case amal::Operator::NotEqual:
    return "NOT_EQUAL";
  case amal::Operator::And:
    return "AND";
  case amal::Operator::Or:
    return "OR";
  case amal::Operator::Xor:
    break;
  }
  return "XOR";
}

const char *opcodeName(amal::Opcode opcode) {
  switch (opcode) {
  case amal::Opcode::Let:
    return "LET";
  case amal::Opcode::Move:
    return "MOVE";
  case amal::Opcode::Anim:
    return "ANIM";
  case amal::Opcode::Jump:
    return "JUMP";
  case amal::Opcode::IfJump:
    return "IF_JUMP";
  case amal::Opcode::For:
    return "FOR";
  case amal::Opcode::Next:
    return "NEXT";
  case amal::Opcode::Pause:
    return "PAUSE";
  case amal::Opcode::Wait:
    return "WAIT";
  case amal::Opcode::End:
    break;
  }
  return "END";
}

std::string versionName(GameVersion version) {
  return version == GameVersion::V12 ? "V12" : "V10";
}

std::string expressionText(const amal::Expression &expression) {
  if (expression.terms == 0) {
    return "{}";
  }
  return "{" + std::to_string(expression.first) + ", " +
         std::to_string(expression.terms) + "}";
}

class TableWriter {
public:
  std::string code(const std::string &name, const std::string &source) {
    const amal::ParsedProgram parsed = amal::parse(source);
    std::vector<uint16_t> code;
    for (const uint16_t index : parsed.code) {
      code.push_back(instruction(parsed, parsed.instructions[index]));
    }
    const auto found = m_codeNames.find(code);
    if (found != m_codeNames.end()) {
      return found->second;
    }
    m_codeNames.emplace(code, name);
    m_codes.emplace_back(name, code);
    return name;
  }

  void function(const std::string &definition) { m_functions += definition; }

  std::string text() const {
    std::ostringstream out;
    out << "#include \"CompiledActors.h\"\n\n"
           "#include <array>\n#include <cstddef>\n#include <cstdint>\n"
           "#include <stdexcept>\n#include <string>\n\n"
           "namespace openfranko::src::engine::street::actors::compiled {\n"
           "namespace {\n\n";
    writeNames(out);
    out << "constexpr std::array<amal::Term, " << m_terms.size()
        << "> TERMS = {{\n";
    for (const amal::Term &term : m_terms) {
      out << "    {" << termKindName(term.kind) << ", " << operatorName(term.op)
          << ", " << term.value << "},\n";
    }
    out << "}};\n\nconstexpr std::array<amal::AnimFrame, " << m_frames.size()
        << "> FRAMES = {{\n";
    for (const amal::AnimFrame &frame : m_frames) {
      out << "    {" << expressionText(frame.image) << ", "
          << expressionText(frame.delay) << "},\n";
    }
    out << "}};\n\nconstexpr std::array<amal::Instruction, "
        << m_instructions.size() << "> INSTRUCTIONS = {{\n";
    for (const amal::Instruction &instruction : m_instructions) {
      out << "    {" << opcodeName(instruction.opcode) << ", "
          << instruction.reg << ", " << expressionText(instruction.first)
          << ", " << expressionText(instruction.second) << ", "
          << expressionText(instruction.third) << ", " << instruction.jump
          << ", " << instruction.firstFrame << ", " << instruction.frames
          << "},\n";
    }
    out << "}};\n";
    for (const auto &[name, code] : m_codes) {
      out << "\nconstexpr std::array<uint16_t, " << code.size() << "> " << name
          << " = {{";
      for (std::size_t i = 0; i < code.size(); ++i) {
        out << (i % NUMBERS_PER_LINE == 0 ? "\n    " : " ") << code[i]
            << (i + 1 < code.size() ? "," : "");
      }
      out << (code.empty() ? "}};\n" : "\n}};\n");
    }
    out << "\ntemplate <std::size_t LENGTH>\n"
           "constexpr amal::Program program(const std::array<uint16_t, "
           "LENGTH> &code) {\n"
           "  return amal::Program{code.data(), INSTRUCTIONS.data(), "
           "TERMS.data(),\n"
           "                       FRAMES.data(), "
           "static_cast<int16_t>(LENGTH)};\n}\n\n"
           "template <typename Value, std::size_t COUNT>\n"
           "std::size_t position(const std::array<Value, COUNT> &values, "
           "Value value,\n"
           "                     const char *what) {\n"
           "  for (std::size_t i = 0; i < COUNT; ++i) {\n"
           "    if (values[i] == value) {\n"
           "      return i;\n"
           "    }\n"
           "  }\n"
           "  throw std::out_of_range(std::string(\"AMAL: no compiled \") + "
           "what);\n}\n\n";
    out << m_tables << "} // namespace\n\n"
        << m_functions
        << "} // namespace openfranko::src::engine::street::actors::compiled\n";
    return out.str();
  }

  void table(const std::string &definition) { m_tables += definition; }

  std::size_t programs() const { return m_codes.size(); }
  std::size_t instructions() const { return m_instructions.size(); }
  std::size_t terms() const { return m_terms.size(); }
  std::size_t frames() const { return m_frames.size(); }

private:
  using TermKey = std::tuple<int, int, int>;
  using FrameKey = std::tuple<int, int, int, int>;
  using InstructionKey =
      std::tuple<int, int, int, int, int, int, int, int, int, int, int>;

  amal::Expression expression(const amal::ParsedProgram &parsed,
                              const amal::Expression &expression) {
    if (expression.terms == 0) {
      return {};
    }
    std::vector<TermKey> key;
    for (uint16_t i = 0; i < expression.terms; ++i) {
      const amal::Term &term = parsed.terms[expression.first + i];
      key.emplace_back(static_cast<int>(term.kind), static_cast<int>(term.op),
                       term.value);
    }
    const auto found = m_expressions.find(key);
    if (found != m_expressions.end()) {
      return {found->second, expression.terms};
    }
    const uint16_t first = static_cast<uint16_t>(m_terms.size());
    for (uint16_t i = 0; i < expression.terms; ++i) {
      m_terms.push_back(parsed.terms[expression.first + i]);
    }
    m_expressions.emplace(key, first);
    return {first, expression.terms};
  }

  uint16_t frameRun(const amal::ParsedProgram &parsed,
                    const amal::Instruction &instruction) {
    std::vector<amal::AnimFrame> run;
    std::vector<FrameKey> key;
    for (uint16_t i = 0; i < instruction.frames; ++i) {
      const amal::AnimFrame &frame = parsed.frames[instruction.firstFrame + i];
      amal::AnimFrame pooled;
      pooled.image = expression(parsed, frame.image);
      pooled.delay = expression(parsed, frame.delay);
      run.push_back(pooled);
      key.emplace_back(pooled.image.first, pooled.image.terms,
                       pooled.delay.first, pooled.delay.terms);
    }
    const auto found = m_frameRuns.find(key);
    if (found != m_frameRuns.end()) {
      return found->second;
    }
    const uint16_t first = static_cast<uint16_t>(m_frames.size());
    m_frames.insert(m_frames.end(), run.begin(), run.end());
    m_frameRuns.emplace(key, first);
    return first;
  }

  uint16_t instruction(const amal::ParsedProgram &parsed,
                       const amal::Instruction &instruction) {
    amal::Instruction pooled = instruction;
    pooled.first = expression(parsed, instruction.first);
    pooled.second = expression(parsed, instruction.second);
    pooled.third = expression(parsed, instruction.third);
    pooled.firstFrame =
        instruction.frames == 0 ? 0 : frameRun(parsed, instruction);
    const InstructionKey key{static_cast<int>(pooled.opcode),
                             pooled.reg,
                             pooled.first.first,
                             pooled.first.terms,
                             pooled.second.first,
                             pooled.second.terms,
                             pooled.third.first,
                             pooled.third.terms,
                             pooled.jump,
                             pooled.firstFrame,
                             pooled.frames};
    const auto found = m_instructionIndices.find(key);
    if (found != m_instructionIndices.end()) {
      return found->second;
    }
    const uint16_t index = static_cast<uint16_t>(m_instructions.size());
    m_instructions.push_back(pooled);
    m_instructionIndices.emplace(key, index);
    return index;
  }

  void writeNames(std::ostringstream &out) const {
    std::set<amal::TermKind> kinds;
    std::set<amal::Operator> usedOperators;
    for (const amal::Term &term : m_terms) {
      kinds.insert(term.kind);
      usedOperators.insert(term.op);
    }
    std::set<amal::Opcode> usedOpcodes;
    for (const amal::Instruction &instruction : m_instructions) {
      usedOpcodes.insert(instruction.opcode);
    }
    const std::vector<std::pair<amal::TermKind, const char *>> termKinds = {
        {amal::TermKind::Number, "Number"},
        {amal::TermKind::Register, "Register"},
        {amal::TermKind::Joystick, "Joystick"},
        {amal::TermKind::Operator, "Operator"}};
    for (const auto &[kind, name] : termKinds) {
      if (kinds.count(kind) != 0) {
        out << "constexpr auto " << termKindName(kind)
            << " = amal::TermKind::" << name << ";\n";
      }
    }
    const std::vector<std::pair<amal::Operator, const char *>> operators = {
        {amal::Operator::Add, "Add"},
        {amal::Operator::Subtract, "Subtract"},
        {amal::Operator::Multiply, "Multiply"},
        {amal::Operator::Divide, "Divide"},
        {amal::Operator::Equal, "Equal"},
        {amal::Operator::Less, "Less"},
        {amal::Operator::Greater, "Greater"},
        {amal::Operator::NotEqual, "NotEqual"},
        {amal::Operator::And, "And"},
        {amal::Operator::Or, "Or"},
        {amal::Operator::Xor, "Xor"}};
    for (const auto &[op, name] : operators) {
      if (usedOperators.count(op) != 0) {
        out << "constexpr auto " << operatorName(op)
            << " = amal::Operator::" << name << ";\n";
      }
    }
    const std::vector<std::pair<amal::Opcode, const char *>> opcodes = {
        {amal::Opcode::Let, "Let"},       {amal::Opcode::Move, "Move"},
        {amal::Opcode::Anim, "Anim"},     {amal::Opcode::Jump, "Jump"},
        {amal::Opcode::IfJump, "IfJump"}, {amal::Opcode::For, "For"},
        {amal::Opcode::Next, "Next"},     {amal::Opcode::Pause, "Pause"},
        {amal::Opcode::Wait, "Wait"},     {amal::Opcode::End, "End"}};
    for (const auto &[opcode, name] : opcodes) {
      if (usedOpcodes.count(opcode) != 0) {
        out << "constexpr auto " << opcodeName(opcode)
            << " = amal::Opcode::" << name << ";\n";
      }
    }
    out << "\n";
  }

  std::vector<amal::Term> m_terms;
  std::map<std::vector<TermKey>, uint16_t> m_expressions;
  std::vector<amal::AnimFrame> m_frames;
  std::map<std::vector<FrameKey>, uint16_t> m_frameRuns;
  std::vector<amal::Instruction> m_instructions;
  std::map<InstructionKey, uint16_t> m_instructionIndices;
  std::vector<std::pair<std::string, std::vector<uint16_t>>> m_codes;
  std::map<std::vector<uint16_t>, std::string> m_codeNames;
  std::string m_tables;
  std::string m_functions;
};

std::string programCall(const std::string &name) {
  return "program(" + name + ")";
}

void single(TableWriter &writer, const std::string &function,
            const std::string &name, const std::string &source) {
  writer.function("amal::Program " + function + "() {\n  return " +
                  programCall(writer.code(name, source)) + ";\n}\n\n");
}

void byVersion(TableWriter &writer, const std::string &function,
               const std::string &name,
               const std::function<std::string(GameVersion)> &source) {
  std::string table = "constexpr std::array<amal::Program, " +
                      std::to_string(actors::compiled::VERSIONS.size()) + "> " +
                      name + "_PROGRAMS = {{\n";
  for (const GameVersion version : actors::compiled::VERSIONS) {
    table += "    " +
             programCall(writer.code(name + "_" + versionName(version),
                                     source(version))) +
             ",\n";
  }
  writer.table(table + "}};\n\n");
  writer.function("amal::Program " + function +
                  "(GameVersion version) {\n  return " + name +
                  "_PROGRAMS[position(VERSIONS, version, \"" + function +
                  " version\")];\n}\n\n");
}

template <typename Values>
void byNumber(TableWriter &writer, const std::string &function,
              const std::string &name, const std::string &parameter,
              const std::string &suffix, const Values &values,
              const std::string &valuesName,
              const std::function<std::string(int)> &source) {
  std::string table = "constexpr std::array<amal::Program, " +
                      std::to_string(values.size()) + "> " + name +
                      "_PROGRAMS = {{\n";
  for (const int value : values) {
    table += "    " +
             programCall(writer.code(
                 name + "_" + suffix + std::to_string(value), source(value))) +
             ",\n";
  }
  writer.table(table + "}};\n\n");
  writer.function("amal::Program " + function + "(int " + parameter +
                  ") {\n  return " + name + "_PROGRAMS[position(" + valuesName +
                  ", " + parameter + ", \"" + function + " " + parameter +
                  "\")];\n}\n\n");
}

std::string playerRow(TableWriter &writer, const std::string &name,
                      const actors::PlayerPrograms &sources) {
  return "    {" +
         programCall(writer.code(name + "_LOCOMOTION", sources.locomotion)) +
         ",\n     " +
         programCall(writer.code(name + "_DAMAGE", sources.damage)) +
         ",\n     " + programCall(writer.code(name + "_CLAMP", sources.clamp)) +
         "},\n";
}

std::string enemyRow(TableWriter &writer, const std::string &name,
                     const actors::EnemyPrograms &sources) {
  return "    {" + programCall(writer.code(name + "_WALK", sources.walk)) +
         ",\n     " +
         programCall(writer.code(name + "_DAMAGE", sources.damage)) + "},\n";
}

std::string dialogueRow(TableWriter &writer, const std::string &name,
                        const actors::DialoguePrograms &sources) {
  return "    {" + programCall(writer.code(name + "_PLAYER", sources.player)) +
         ",\n     " + programCall(writer.code(name + "_BOSS", sources.boss)) +
         "},\n";
}

void writeTables(TableWriter &writer) {
  const auto &versions = actors::compiled::VERSIONS;
  const auto &stages = actors::compiled::STAGES;

  single(writer, "playerBlood", "PLAYER_BLOOD", actors::playerBlood());
  byVersion(writer, "enemyBlood", "ENEMY_BLOOD", actors::enemyBlood);
  byVersion(writer, "screenShake", "SCREEN_SHAKE", actors::screenShake);

  std::string table = "constexpr std::array<PlayerPrograms, " +
                      std::to_string(stages.size() * versions.size()) +
                      "> STREET_PLAYER_PROGRAMS = {{\n";
  for (const int stage : stages) {
    for (const GameVersion version : versions) {
      table += playerRow(writer,
                         "STREET_PLAYER_STAGE" + std::to_string(stage) + "_" +
                             versionName(version),
                         actors::streetPlayer(stage, version));
    }
  }
  writer.table(table + "}};\n\n");
  writer.function(
      "PlayerPrograms streetPlayer(int stage, GameVersion version) {\n"
      "  return STREET_PLAYER_PROGRAMS\n"
      "      [position(STAGES, stage, \"streetPlayer stage\") * "
      "VERSIONS.size() +\n"
      "       position(VERSIONS, version, \"streetPlayer version\")];\n}\n\n");

  const auto &bases = actors::compiled::ENEMY_IMAGE_BASES;
  const auto &types = actors::compiled::ENEMY_TYPES;
  table = "constexpr std::array<EnemyPrograms, " +
          std::to_string(bases.size() * types.size() * versions.size()) +
          "> ENEMY_PROGRAMS = {{\n";
  for (const int base : bases) {
    for (const int type : types) {
      for (const GameVersion version : versions) {
        table += enemyRow(writer,
                          "ENEMY_BASE" + std::to_string(base) + "_TYPE" +
                              std::to_string(type) + "_" + versionName(version),
                          actors::enemy(base, type, version));
      }
    }
  }
  writer.table(table + "}};\n\n");
  writer.function(
      "EnemyPrograms enemy(int imageBase, int type, GameVersion version) {\n"
      "  const std::size_t base =\n"
      "      position(ENEMY_IMAGE_BASES, imageBase, \"enemy image base\");\n"
      "  const std::size_t kind = position(ENEMY_TYPES, type, \"enemy "
      "type\");\n"
      "  return ENEMY_PROGRAMS[(base * ENEMY_TYPES.size() + kind) * "
      "VERSIONS.size() +\n"
      "                        position(VERSIONS, version, \"enemy "
      "version\")];\n}\n\n");

  byVersion(writer, "idle", "IDLE", actors::idle);
  byNumber(writer, "indicatorArrow", "INDICATOR_ARROW", "facing", "FACING",
           actors::compiled::ARROW_FACINGS, "ARROW_FACINGS",
           actors::indicatorArrow);

  table = "constexpr std::array<PlayerPrograms, " +
          std::to_string(stages.size()) + "> BOSS_PLAYER_PROGRAMS = {{\n";
  for (const int stage : stages) {
    table += playerRow(writer, "BOSS_PLAYER_STAGE" + std::to_string(stage),
                       actors::bossPlayer(stage));
  }
  writer.table(table + "}};\n\n");
  writer.function("PlayerPrograms bossPlayer(int stage) {\n"
                  "  return BOSS_PLAYER_PROGRAMS[position(STAGES, stage, "
                  "\"bossPlayer stage\")];\n}\n\n");

  table = "constexpr std::array<EnemyPrograms, " +
          std::to_string(stages.size()) + "> BOSS_PROGRAMS = {{\n";
  for (const int stage : stages) {
    table += enemyRow(writer, "BOSS_STAGE" + std::to_string(stage),
                      actors::boss(stage));
  }
  writer.table(table + "}};\n\n");
  writer.function("EnemyPrograms boss(int stage) {\n"
                  "  return BOSS_PROGRAMS[position(STAGES, stage, \"boss "
                  "stage\")];\n}\n\n");

  byNumber(writer, "spectator", "SPECTATOR", "stage", "STAGE", stages, "STAGES",
           actors::spectator);

  table = "constexpr std::array<DialoguePrograms, " +
          std::to_string(stages.size()) + "> DIALOGUE_PROGRAMS = {{\n";
  for (const int stage : stages) {
    table += dialogueRow(writer, "DIALOGUE_STAGE" + std::to_string(stage),
                         actors::dialogue(stage));
  }
  writer.table(table + "}};\n\n");
  writer.function("DialoguePrograms dialogue(int stage) {\n"
                  "  return DIALOGUE_PROGRAMS[position(STAGES, stage, "
                  "\"dialogue stage\")];\n}\n\n");

  single(writer, "walkToBoss", "WALK_TO_BOSS", actors::walkToBoss());
  single(writer, "finishingPose", "FINISHING_POSE", actors::finishingPose());
  single(writer, "finishingBlood", "FINISHING_BLOOD", actors::finishingBlood());
  single(writer, "finishingPoseBack", "FINISHING_POSE_BACK",
         actors::finishingPoseBack());
  single(writer, "walkOff", "WALK_OFF", actors::walkOff());
  single(writer, "bossThrown", "BOSS_THROWN", actors::bossThrown());
  single(writer, "victoryLift", "VICTORY_LIFT", actors::victoryLift());
  single(writer, "bossRests", "BOSS_RESTS", actors::bossRests());
  single(writer, "bubbleUntilFire", "BUBBLE_UNTIL_FIRE",
         actors::bubbleUntilFire());
  single(writer, "walkAway", "WALK_AWAY", actors::walkAway());
  single(writer, "breakDance", "BREAK_DANCE", actors::breakDance());
  byNumber(writer, "portraitEntrance", "PORTRAIT_ENTRANCE", "portrait",
           "PORTRAIT", actors::compiled::PORTRAITS, "PORTRAITS",
           actors::portraitEntrance);
  single(writer, "danceFinale", "DANCE_FINALE", actors::danceFinale());
  byNumber(writer, "portraitShuttle", "PORTRAIT_SHUTTLE", "portrait",
           "PORTRAIT", actors::compiled::PORTRAITS, "PORTRAITS",
           actors::portraitShuttle);
  single(writer, "pointingHand", "POINTING_HAND", actors::pointingHand());
  byNumber(writer, "pedestrian", "PEDESTRIAN", "image", "IMAGE",
           actors::compiled::PEDESTRIAN_IMAGES, "PEDESTRIAN_IMAGES",
           actors::pedestrian);
  single(writer, "carDriveOff", "CAR_DRIVE_OFF", actors::carDriveOff());
}

} // namespace

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);
  const auto outputOption = parser.option("-o");
  if (argc > 1 && !outputOption.has_value()) {
    std::cerr << "Usage: " << argv[0] << " [-o <output_file>]" << std::endl;
    std::cerr << "Compiles the AMAL programs of street/actors into the tables "
                 "of street/actors/compiled/CompiledActors.cpp."
              << std::endl;
    return 1;
  }
  const std::string outputPath = outputOption.value_or("CompiledActors.cpp");
  try {
    TableWriter writer;
    writeTables(writer);
    const std::string text = writer.text();
    filesystem::writeFile::writeFile(
        outputPath, std::vector<uint8_t>(text.begin(), text.end()));
    std::cerr << "Compiled " << writer.programs() << " programs into "
              << writer.instructions() << " instructions, " << writer.terms()
              << " terms and " << writer.frames() << " frames" << std::endl;
    std::cerr << "Wrote " << outputPath << " (" << text.size() << " bytes)"
              << std::endl;
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << std::endl;
    return 1;
  }
  return 0;
}
