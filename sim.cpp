#include <iostream>

enum {
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
  SYS
};

unsigned pc;
unsigned sp;
unsigned areg;
unsigned breg;
unsigned oreg;

unsigned inst;

// 256KB memory.
unsigned char mem[1<<16];

bool running;

void syscall() {
}

void run() {
  while (running) {
    inst = mem[pc];
    switch ((inst >> 4) & 0xF) {
    case LDAM:
      areg = mem[oreg];
      oreg = 0;
      break;
    case LDBM:
      breg = mem[oreg];
      oreg = 0;
      break;
    case STAM:
      mem[oreg] = areg;
      oreg = 0;
      break;
    case LDAC:
      areg = oreg;
      oreg = 0;
      break;
    case LDBC:
      breg = oreg;
      oreg = 0;
      break;
    case LDAP:
      areg = pc + oreg;
      oreg = 0;
      break;
    case LDAI:
      areg = mem[areg + oreg];
      oreg = 0;
      break;
    case LDBI:
      breg = mem[breg + oreg];
      oreg = 0;
      break;
    case STAI:
      mem[breg + oreg] = areg;
      oreg = 0;
      break;
    case BR:
      pc = pc + oreg;
      oreg = 0;
      break;
    case BRZ:
      if (areg == 0) {
        pc = pc + oreg;
      }
      oreg = 0;
      break;
    case BRN:
      if ((int) areg < 0) {
        pc = pc + oreg;
      }
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
        oreg = 0;
        break;
      case ADD:
        areg = areg + breg;
        oreg = 0;
        break;
      case SUB:
        areg = areg - breg;
        oreg = 0;
        break;
      case SYS:
        syscall();
        break;
      };
      oreg = 0;
      break;
    }
  }
}

int main(int argc, int *argv[]) {
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " executable\n";
    return 1;
  }
  load(argv[1]);
  return run();
}

