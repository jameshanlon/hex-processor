#include <array>
#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <exception>
#include <boost/format.hpp>

#include "Hex.hpp"
#include "Util.hpp"

class Processor {

  // Constants.
  static const size_t MEMORY_SIZE_WORDS = 200000;

  // State.
  uint32_t pc;
  uint32_t areg;
  uint32_t breg;
  uint32_t oreg;
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
  hex::Instr instrEnum;

public:

  Processor() :
    pc(0), areg(0), breg(0), oreg(0), running(true), tracing(false), cycles(0) {}

  void setTracing(bool value) { tracing = value; }

  void load(const char *filename, bool dumpContents) {
    // Load the binary file.
    std::streampos fileSize;
    std::ifstream file(filename, std::ios::binary);

    // Get length of file.
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read the contents.
    file.read(reinterpret_cast<char*>(memory.data()), fileSize);

    // Print the contents of the binary.
    if (dumpContents) {
      std::cout << "Read " << std::to_string(fileSize) << " bytes\n";
      for (size_t i=0; i<(fileSize / 4) + 1; i++) {
        std::cout << boost::format("%08d %08x\n") % i % memory[i];
      }
    }
  }

  void traceSyscall() {
    unsigned spWordIndex = memory[1];
    std::cout << "spWordIndex="<<spWordIndex<<"\n";
    switch (static_cast<hex::Syscall>(areg)) {
      case hex::Syscall::EXIT:
        std::cout << "exit\n";
        break;
      case hex::Syscall::WRITE:
        std::cout << boost::format("write %d to simin(%d)\n") % memory[spWordIndex+2] % memory[spWordIndex+3];
        break;
      case hex::Syscall::READ:
        std::cout << boost::format("read to mem[%08x]\n") % (spWordIndex+1);
        break;
    }
  }

  void trace(uint32_t instr, hex::Instr instrEnum) {
    std::cout << boost::format("%-6d %-6s %-4d") % pc % instrEnumToStr(instrEnum) % (instr & 0xF);
    switch (instrEnum) {
      case hex::Instr::LDAM:
        std::cout << boost::format("areg = mem[oreg (%#08x)] (%d)\n") % oreg % memory[oreg];
        break;
      case hex::Instr::LDBM:
        std::cout << boost::format("breg = mem[oreg (%#08x)] (%d)\n") % oreg % memory[oreg];
        break;
      case hex::Instr::STAM:
        std::cout << boost::format("mem[oreg (%#08x)] = areg %d\n") % oreg % areg;
        break;
      case hex::Instr::LDAC:
        std::cout << boost::format("areg = oreg %d\n") % oreg;
        break;
      case hex::Instr::LDBC:
        std::cout << boost::format("breg = oreg %d\n") % oreg;
        break;
      case hex::Instr::LDAP:
        std::cout << boost::format("areg = pc (%d) + oreg (%d) %d\n") % pc % oreg % (pc + oreg);
        break;
      case hex::Instr::LDAI:
        std::cout << boost::format("areg = mem[areg (%d) + oreg (%d) = %#08x] (%d)\n") % areg % oreg % (areg+oreg) % memory[areg+oreg];
        break;
      case hex::Instr::LDBI:
        std::cout << boost::format("breg = mem[breg (%d) + oreg (%d) = %#08x] (%d)\n") % breg % oreg % (breg+oreg) % memory[breg+oreg];
        break;
      case hex::Instr::STAI:
        std::cout << boost::format("mem[breg (%d) + oreg (%d) = %#08x] = areg (%d)\n") % breg % oreg % (breg+oreg) % areg;
        break;
      case hex::Instr::BR:
        std::cout << boost::format("pc = pc + oreg (%d) (%#08x)\n") % oreg % (pc + oreg);
        break;
      case hex::Instr::BRZ:
        std::cout << boost::format("pc = areg == zero ? pc + oreg (%d) (%#08x) : pc\n") % oreg % (pc + oreg);
        break;
      case hex::Instr::BRN:
        std::cout << boost::format("pc = areg < zero ? pc + oreg (%d) (%#08x) : pc\n") % oreg % (pc + oreg);
        break;
      case hex::Instr::PFIX:
        std::cout << boost::format("oreg = oreg (%d) << 4 (%#08x)\n") % oreg % (oreg << 4);
        break;
      case hex::Instr::NFIX:
        std::cout << boost::format("oreg = 0xFFFFFF00 | oreg (%d) << 4 (%#08x)\n") % oreg % (0xFFFFFF00 | (oreg << 4));
        break;
      case hex::Instr::OPR:
        switch (static_cast<hex::OprInstr>(oreg)) {
          case hex::OprInstr::BRB:
            std::cout << boost::format("pc = breg (%#08x)\n") % breg;
            break;
          case hex::OprInstr::ADD:
           std::cout << boost::format("areg = areg (%d) + breg (%d) (%d)\n") % areg % breg % (areg + breg);
            break;
          case hex::OprInstr::SUB:
           std::cout << boost::format("areg = areg (%d) - breg (%d) (%d)\n") % areg % breg % (areg - breg);
            break;
          case hex::OprInstr::SVC:
            traceSyscall();
            break;
        };
        break;
   }
  }

  void syscall() {
    unsigned spWordIndex = memory[1];
    switch (static_cast<hex::Syscall>(areg)) {
      case hex::Syscall::EXIT:
        running = false;
        break;
      case hex::Syscall::WRITE:
        hex::output(fileIO, memory[spWordIndex+2], memory[spWordIndex+3]);
        break;
      case hex::Syscall::READ:
        memory[spWordIndex+1] = hex::input(fileIO, memory[spWordIndex+2]);
        break;
      default:
        throw std::runtime_error("invalid syscall: " + std::to_string(areg));
    }
  }

  void run() {
    while (running) {
      instr = (memory[pc >> 2] >> ((pc & 0x3) << 3)) & 0xFF;
      pc = pc + 1;
      oreg = oreg | (instr & 0xF);
      instrEnum = static_cast<hex::Instr>((instr >> 4) & 0xF);
      if (tracing) {
        trace(instr, instrEnum);
      }
      switch (instrEnum) {
        case hex::Instr::LDAM:
          areg = memory[oreg];
          oreg = 0;
          break;
        case hex::Instr::LDBM:
          breg = memory[oreg];
          oreg = 0;
          break;
        case hex::Instr::STAM:
          memory[oreg] = areg;
          oreg = 0;
          break;
        case hex::Instr::LDAC:
          areg = oreg;
          oreg = 0;
          break;
        case hex::Instr::LDBC:
          breg = oreg;
          oreg = 0;
          break;
        case hex::Instr::LDAP:
          areg = pc + oreg;
          oreg = 0;
          break;
        case hex::Instr::LDAI:
          areg = memory[areg + oreg];
          oreg = 0;
          break;
        case hex::Instr::LDBI:
          breg = memory[breg + oreg];
          oreg = 0;
          break;
        case hex::Instr::STAI:
          memory[breg + oreg] = areg;
          oreg = 0;
          break;
        case hex::Instr::BR:
          pc = pc + oreg;
          oreg = 0;
          break;
        case hex::Instr::BRZ:
          if (areg == 0) {
            pc = pc + oreg;
          }
          oreg = 0;
          break;
        case hex::Instr::BRN:
          if ((int)areg < 0) {
            pc = pc + oreg;
          }
          oreg = 0;
          break;
        case hex::Instr::PFIX:
          oreg = oreg << 4;
          break;
        case hex::Instr::NFIX:
          oreg = 0xFFFFFF00 | (oreg << 4);
          break;
        case hex::Instr::OPR:
          switch (static_cast<hex::OprInstr>(oreg)) {
            case hex::OprInstr::BRB:
              pc = breg;
              oreg = 0;
              break;
            case hex::OprInstr::ADD:
              areg = areg + breg;
              oreg = 0;
              break;
            case hex::OprInstr::SUB:
              areg = areg - breg;
              oreg = 0;
              break;
            case hex::OprInstr::SVC:
              syscall();
              break;
            default:
              throw std::runtime_error("invalid OPR: " + std::to_string(oreg));
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
  std::cout << "  -d,--dump  Dump the binary file contents\n";
  std::cout << "  -t,--trace Enable instruction tracing\n";
}

int main(int argc, const char *argv[]) {
  try {
    const char *filename = nullptr;
    bool dumpBinary = false;
    bool trace = false;
    for (unsigned i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-d") == 0 ||
          std::strcmp(argv[i], "--dump") == 0) {
        dumpBinary = true;
      } else if (std::strcmp(argv[i], "-t") == 0 ||
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
    p.load(filename, dumpBinary);
    if (dumpBinary) {
      return 0;
    }
    p.run();
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
