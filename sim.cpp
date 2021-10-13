#include <array>
#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <exception>
#include <boost/format.hpp>

#include "Instructions.hpp"

class Processor {

  // Constants.
  static const size_t MEMORY_SIZE_WORDS = 200000;

  // State.
  uint32_t pc;
  uint32_t sp;
  uint32_t areg;
  uint32_t breg;
  uint32_t oreg;
  uint32_t temp;
  uint32_t instr;

  // Memory.
  std::array<uint32_t, MEMORY_SIZE_WORDS> memory;

  // IO
  std::vector<std::fstream> fileIO;

  // Control.
  bool running;
  bool tracing;

  // State for tracing.
  uint64_t cycles;
  Instr instrEnum;
  OprInstr oprInstrEnum;
  uint32_t lastPC;

public:

  Processor() :
    tracing(false), cycles(0) {}

  void setTracing(bool value) { tracing = value; }

  void load(const char *filename) {
    // Load the binary file.
    std::streampos fileSize;
    std::ifstream file(filename, std::ios::binary);

    // Get length of file.
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read the contents.
    file.read(reinterpret_cast<char*>(memory.data()), fileSize);
    std::cout << "Read " << std::to_string(fileSize) << " bytes\n";
    //for (size_t i=0; i<(fileSize / 4) + 1; i++) {
    //  std::cout << boost::format("%08d %08x\n") % i % memory[i];
    //}
  }

  void trace() {
    if (instrEnum == Instr::OPR) {
      auto oprInstrOpc = oprInstrEnumToStr(oprInstrEnum);
      std::cout << boost::format("@%-6d %8d %#8x %6s %8s %8x %8x\n")
                   % cycles % lastPC % instr % "OPR" % oprInstrOpc % areg % breg;
    } else {
      auto instrOpc = instrEnumToStr(instrEnum);
      std::cout << boost::format("@%-6d %8d %#8x %6s %8x %8x %8x\n")
                     % cycles % lastPC % instr % instrOpc % oreg % areg % breg;
    }
  }

  /// Output a character to stdout or a file.
  void output(char value, int stream) {
    if (stream < 256) {
      std::cout << value;
    } else {
      size_t index = (stream >> 8) & 7;
      if (!fileIO[index].is_open()) {
        fileIO[index].open(std::string("simin")+std::to_string(index),
                           std::fstream::in);
      }
      fileIO[index].put(value);
    }
  }

  /// Input a character from stdin or a file.
  char input(int stream) {
    if (stream < 256) {
      return std::getchar();
    } else {
      size_t index = (stream >> 8) & 7;
      if (!fileIO[index].is_open()) {
        fileIO[index].open(std::string("simin")+std::to_string(index),
                           std::fstream::out);
      }
      return fileIO[index].get();
    }
  }

  void syscall() {
    sp = memory[1];
    switch (static_cast<Syscall>(areg)) {
      case Syscall::EXIT:
        running = false;
        break;
      case Syscall::WRITE:
        output(memory[sp+2], memory[sp+3]);
        break;
      case Syscall::READ:
        memory[sp+1] = input(memory[sp+2]);
        break;
    }
  }

  void run() {
    while (running) {
      instr = (memory[pc >> 2] >> ((pc & 0x3) << 3)) & 0xFF;
      lastPC = pc;
      pc = pc + 1;
      oreg = oreg | (instr & 0xF);
      instrEnum = static_cast<Instr>((instr >> 4) & 0xF);
      if (tracing) {
        trace();
      }
      switch (instrEnum) {
        case Instr::LDAM:
          areg = memory[oreg];
          oreg = 0;
          break;
        case Instr::LDBM:
          breg = memory[oreg];
          oreg = 0;
          break;
        case Instr::STAM:
          memory[oreg] = areg;
          oreg = 0;
          break;
        case Instr::LDAC:
          areg = oreg;
          oreg = 0;
          break;
        case Instr::LDBC:
          breg = oreg;
          oreg = 0;
          break;
        case Instr::LDAP:
          areg = pc + oreg;
          oreg = 0;
          break;
        case Instr::LDAI:
          temp = areg;
          areg = memory[areg + oreg];
          oreg = 0;
          break;
        case Instr::LDBI:
          breg = memory[breg + oreg];
          oreg = 0;
          break;
        case Instr::STAI:
          memory[breg + oreg] = areg;
          oreg = 0;
          break;
        case Instr::BR:
          pc = pc + oreg;
          oreg = 0;
          break;
        case Instr::BRZ:
          if (areg == 0) {
            pc = pc + oreg;
          }
          oreg = 0;
          break;
        case Instr::BRN:
          if ((int)areg < 0) {
            pc = pc + oreg;
          }
          oreg = 0;
          break;
        case Instr::PFIX:
          oreg = oreg << 4;
          break;
        case Instr::NFIX:
          oreg = 0xFFFFFF00 | (oreg << 4);
          break;
        case Instr::OPR:
          oprInstrEnum = static_cast<OprInstr>(oreg);
          switch (oprInstrEnum) {
            case OprInstr::BRB:
              pc = breg;
              oreg = 0;
              break;
            case OprInstr::ADD:
              temp = areg;
              areg = areg + breg;
              oreg = 0;
              break;
            case OprInstr::SUB:
              temp = areg;
              areg = areg - breg;
              oreg = 0;
              break;
            case OprInstr::SVC:
              syscall();
              break;
            default:
              throw std::runtime_error("invalid syscall: " + std::to_string(oreg));
          };
          oreg = 0;
          break;
        default:
          throw std::runtime_error("invalid instruction");
      }
      cycles++;
    }
  }
};

static void help(const char *argv[]) {
  std::cout << "Hex processor simulator\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
  std::cout << "Positional arguments:\n";
  std::cout << "  file A binary file to simulate\n\n";
  std::cout << "Optional arguments:\n";
  std::cout << "  -h,--help  Display this message\n";
  std::cout << "  -t,--trace Enable instruction tracing\n";
}

int main(int argc, const char *argv[]) {
  try {
    const char *filename = nullptr;
    bool trace = false;
    for (unsigned i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-t") == 0 ||
          std::strcmp(argv[i], "--trace") == 0) {
        trace = true;
      } else if (std::strcmp(argv[i], "-h") == 0 ||
                 std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        return 1;
      } else {
        if (!filename) {
          filename = argv[i];
        } else {
          throw std::runtime_error("cannot specify more than one file");
        }
      }
    }
    // A file must be specified.
    if (!filename) {
      help(argv);
      return 1;
    }
    Processor p;
    p.setTracing(trace);
    p.load(filename);
    p.run();
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
