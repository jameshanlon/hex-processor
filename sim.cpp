#include <cstdio>
#include <cstdint>
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

  void trace1(const char *opc, uint32_t op1, uint32_t op2) {
    if (tracing) {
      std::cout << opc << " " << op1
                << " @ " << cycles << '\n';
    }
  }

  void trace2(const char *opc, uint32_t op1, uint32_t op2) {
    if (tracing) {
      std::cout << opc << " " << op1 << ", " << op2
                << " @ " << cycles << '\n';
    }
  }

  void trace3(const char *opc, uint32_t op1, uint32_t op2, uint32_t op3) {
    if (tracing) {
      std::cout << opc << " " << op1 << ", " << op2 << ", " << op3
                << " @ " << cycles << '\n';
    }
  }

  void run() {
    while (running) {
      inst = mem[pc];
      switch ((inst >> 4) & 0xF) {
      case LDAM:
        areg = mem[oreg];
        trace2("LDAM", areg, oreg);
        oreg = 0;
        break;
      case LDBM:
        breg = mem[oreg];
        trace2("LDBM", areg, oreg);
        oreg = 0;
        break;
      case STAM:
        mem[oreg] = areg;
        trace2("STAM", areg, oreg);
        oreg = 0;
        break;
      case LDAC:
        areg = oreg;
        trace1("LDAC", oreg);
        oreg = 0;
        break;
      case LDBC:
        breg = oreg;
        trace1("LDBC", oreg);
        oreg = 0;
        break;
      case LDAP:
        areg = pc + oreg;
        trace2("LDAP", areg, oreg);
        oreg = 0;
        break;
      case LDAI:
        temp = areg
        areg = mem[areg + oreg];
        trace3("LDAI", areg, temp, oreg);
        oreg = 0;
        break;
      case LDBI:
        breg = mem[breg + oreg];
        trace3("LDBI", breg, areg, oreg);
        oreg = 0;
        break;
      case STAI:
        mem[breg + oreg] = areg;
        trace3("STAI", breg, oreg, areg);
        oreg = 0;
        break;
      case BR:
        pc = pc + oreg;
        trace1("BR", oreg);
        oreg = 0;
        break;
      case BRZ:
        if (areg == 0) {
          pc = pc + oreg;
        }
        trace2("BRZ", areg, oreg);
        oreg = 0;
        break;
      case BRN:
        if ((int) areg < 0) {
          pc = pc + oreg;
        }
        trace2("BRN", areg, oreg);
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
          trace1("BRB", breg);
          oreg = 0;
          break;
        case ADD:
          temp = areg;
          areg = areg + breg;
          trace1("ADD", areg, temp, breg);
          oreg = 0;
          break;
        case SUB:
          temp = areg;
          areg = areg - breg;
          trace1("SUB", areg, temp, breg);
          oreg = 0;
          break;
        case SYS:
          trace1("SYS", areg);
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
    if (std::strcmp(argv[i], "-h") == 0) {
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

