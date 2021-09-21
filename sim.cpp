#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <exception>
#include <boost/format.hpp>

class Processor {

  static const size_t MEMORY_SIZE_BYTES = 1 << 16;

  enum class Instr {
    LDAM,
    LDBM,
    STAM,
    LDAC,
    LDBC,
    LDAP,
    LDAI,
    LDBI,
    STAI,
    BR,
    BRZ,
    BRN,
    BRB,
    ADD,
    SUB,
    DIV,
    MUL,
    REM,
    SYS,
    PFIX,
    NFIX,
    OPR
  };

  enum class Syscall {
    EXIT,
    WRITE,
    READ,
  };

  uint32_t pc;
  uint32_t sp;
  uint32_t areg;
  uint32_t breg;
  uint32_t oreg;
  uint32_t temp;
  uint32_t inst;

  // 256KB memory.
  char memory[MEMORY_SIZE_BYTES];

  uint64_t cycles;
  bool running;
  bool tracing;

public:

  Processor() :
    cycles(0),
    tracing(false) {}

  void setTracing(bool value) { tracing = value; }

  void load(const char *filename) {
    // Load the binary file.
    std::ifstream file(filename, std::ios::binary);
    if (file) {
      // Get length of file.
      file.seekg(0, file.end);
      int length = file.tellg();
      file.seekg(0, file.beg);
      // Read the contents.
      file.read(static_cast<char*>(memory), length);
      if (!file) {
        throw std::runtime_error("could not read all of the file");
      }
    }
    throw std::runtime_error("could not open the file");
  }

  void syscall() {
    switch (static_cast<Syscall>(areg)) {
      case Syscall::EXIT:
        running = false;
        break;
      case Syscall::WRITE:
        std::putchar(memory[sp]);
        break;
      case Syscall::READ:
        memory[sp] = std::getchar();
        break;
    }
  }

  void trace(const char *opc, uint32_t op1) {
    if (!tracing) {
      return;
    }
    std::cout << boost::format("%d %s %d\n") % cycles % opc % op1;
  }

  void trace(const char *opc, uint32_t op1, uint32_t op2) {
    if (!tracing) {
      return;
    }
    std::cout << boost::format("%d %s %d %d\n") % cycles % opc % op1 % op2;
  }

  void trace(const char *opc, uint32_t op1, uint32_t op2, uint32_t op3) {
    if (!tracing) {
      return;
    }
    std::cout << boost::format("%d %s %d %d\n") % cycles % opc % op1 % op2 % op3;
  }

  void run() {
    while (running) {
      inst = memory[pc];
      switch (static_cast<Instr>((inst >> 4) & 0xF)) {
        case Instr::LDAM:
          areg = memory[oreg];
          trace("LDAM", areg, oreg);
          oreg = 0;
          break;
        case Instr::LDBM:
          breg = memory[oreg];
          trace("LDBM", areg, oreg);
          oreg = 0;
          break;
        case Instr::STAM:
          memory[oreg] = areg;
          trace("STAM", areg, oreg);
          oreg = 0;
          break;
        case Instr::LDAC:
          areg = oreg;
          trace("LDAC", oreg);
          oreg = 0;
          break;
        case Instr::LDBC:
          breg = oreg;
          trace("LDBC", oreg);
          oreg = 0;
          break;
        case Instr::LDAP:
          areg = pc + oreg;
          trace("LDAP", areg, oreg);
          oreg = 0;
          break;
        case Instr::LDAI:
          temp = areg;
          areg = memory[areg + oreg];
          trace("LDAI", areg, temp, oreg);
          oreg = 0;
          break;
        case Instr::LDBI:
          breg = memory[breg + oreg];
          trace("LDBI", breg, areg, oreg);
          oreg = 0;
          break;
        case Instr::STAI:
          memory[breg + oreg] = areg;
          trace("STAI", breg, oreg, areg);
          oreg = 0;
          break;
        case Instr::BR:
          pc = pc + oreg;
          trace("BR", oreg);
          oreg = 0;
          break;
        case Instr::BRZ:
          if (areg == 0) {
            pc = pc + oreg;
          }
          trace("BRZ", areg, oreg);
          oreg = 0;
          break;
        case Instr::BRN:
          if ((int) areg < 0) {
            pc = pc + oreg;
          }
          trace("BRN", areg, oreg);
          oreg = 0;
          break;
        case Instr::PFIX:
          oreg = oreg << 4;
          break;
        case Instr::NFIX:
          oreg = 0xFFFFFF00 | (oreg << 4);
          break;
        case Instr::OPR:
          switch (static_cast<Instr>(oreg)) {
            case Instr::BRB:
              pc = breg;
              trace("BRB", breg);
              oreg = 0;
              break;
            case Instr::ADD:
              temp = areg;
              areg = areg + breg;
              trace("ADD", areg, temp, breg);
              oreg = 0;
              break;
            case Instr::SUB:
              temp = areg;
              areg = areg - breg;
              trace("SUB", areg, temp, breg);
              oreg = 0;
              break;
            case Instr::SYS:
              trace("SYS", areg);
              syscall();
              break;
            default:
              break;
          };
          oreg = 0;
          break;
        default:
          break;
      }
    }
  }
};

static void help(const char *argv[]) {
  std::cout << "Hex processor simulator\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
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
        std::exit(1);
      } else {
        if (!filename) {
          filename = argv[i];
        } else {
          throw std::runtime_error("cannot specify more than one file");
        }
      }
    }
    if (!filename) {
      help(argv);
      std::exit(1);
    }
    Processor p;
    p.setTracing(trace);
    p.load(filename);
    p.run();
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    std::exit(1);
  }
  return 0;
}

