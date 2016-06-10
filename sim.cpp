#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include <iostream>

static void error(const char *message) {
  std::cout << "Error: " << message << '\n';
  std::exit(1);
}

class Processor {

  enum Instr {
    LDAM = 0,
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
  };

  enum Syscall {
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
  uint8_t mem[1<<16];

  uint64_t cycles;
  bool running;
  bool trace;

public:

  Processor(bool trace) : cycles(0), trace(trace) {}

  void load(const char *filename) {
    // Load the binary file.
    std::ifstream file(filename, std::ios::binary);
    if (file) {
      // Get length of file.
      file.seekg(0, file.end);
      int length = file.tellg();
      file.seekg(0, file.beg);
      // Read the contents.
      file.read(mem, length);
      if (!file) {
        error("could not read all of the file");
      }
    }
    error("could not open the file");
  }

  void syscall() {
    switch (areg) {
    case EXIT:
      running = false;
      break;
    case WRITE:
      std::putchar(mem[sp]);
      break;
    case READ:
      mem[sp] = std::getchar(mem[sp]);
      break;
    }
  }

  void trace(unsigned count, const char *opc, ...) {
    if (tracing) {
      va_list args;
      va_start(args, opc);
      std::cout << opc << " ";
      for (unsigned i = 0; i < count; ++i) {
        std::cout << op1 << i+1 < count ? ", " : "";
      }
      std::cout << " @ " << cycles << '\n';
    }
  }

  void run() {
    while (running) {
      inst = mem[pc];
      switch ((inst >> 4) & 0xF) {
      case LDAM:
        areg = mem[oreg];
        trace(2, "LDAM", areg, oreg);
        oreg = 0;
        break;
      case LDBM:
        breg = mem[oreg];
        trace(2, "LDBM", areg, oreg);
        oreg = 0;
        break;
      case STAM:
        mem[oreg] = areg;
        trace(2, "STAM", areg, oreg);
        oreg = 0;
        break;
      case LDAC:
        areg = oreg;
        trace(1, "LDAC", oreg);
        oreg = 0;
        break;
      case LDBC:
        breg = oreg;
        trace(1, "LDBC", oreg);
        oreg = 0;
        break;
      case LDAP:
        areg = pc + oreg;
        trace(1, "LDAP", areg, oreg);
        oreg = 0;
        break;
      case LDAI:
        temp = areg
        areg = mem[areg + oreg];
        trace(3, "LDAI", areg, temp, oreg);
        oreg = 0;
        break;
      case LDBI:
        breg = mem[breg + oreg];
        trace(3, "LDBI", breg, areg, oreg);
        oreg = 0;
        break;
      case STAI:
        mem[breg + oreg] = areg;
        trace3(3, "STAI", breg, oreg, areg);
        oreg = 0;
        break;
      case BR:
        pc = pc + oreg;
        trace(1, "BR", oreg);
        oreg = 0;
        break;
      case BRZ:
        if (areg == 0) {
          pc = pc + oreg;
        }
        trace(2, "BRZ", areg, oreg);
        oreg = 0;
        break;
      case BRN:
        if ((int) areg < 0) {
          pc = pc + oreg;
        }
        trace(2, "BRN", areg, oreg);
        oreg = 0;
        break;
      case PFIX:
        oreg = oreg << 4;
        break;
      case NFIX:
        oreg = 0xFFFFFF00 | (oreg << 4);
        break;
      case OPR:
        switch (oreg) {
        case BRB:
          pc = breg;
          trace(1, "BRB", breg);
          oreg = 0;
          break;
        case ADD:
          temp = areg;
          areg = areg + breg;
          trace(1, "ADD", areg, temp, breg);
          oreg = 0;
          break;
        case SUB:
          temp = areg;
          areg = areg - breg;
          trace(1, "SUB", areg, temp, breg);
          oreg = 0;
          break;
        case SYS:
          trace(1, "SYS", areg);
          syscall();
          break;
        };
        oreg = 0;
        break;
      }
    }
  }
};

static void help(int *argv[]) {
  std::cout << "Hex processor simulator\n\n";
  std::cout << "Usage: " << argv[0] << " <binary>\n\n";
  std::cout << "Options:\n";
  std::cout << "  -h display this message\n";
  std::cout << "  -t enable tracing\n";
  std::exit(1);
}

int main(int argc, const char *argv[]) {
  const char *filename = nullptr;
  bool trace = false;
  for (unsigned i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "-t") == 0) {
      trace = true;
    } else if (std::strcmp(argv[i], "-h") == 0) {
      help(argv);
    } else {
      if (!filename) {
        filename = argv[i];
      } else {
        error("cannot specify more than one binary");
      }
    }
  }
  if (!filename) {
    help(argv);
  }
  Processor p(trace);
  p.load(filename);
  return p.run();
}

