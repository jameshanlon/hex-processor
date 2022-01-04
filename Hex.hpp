#ifndef HEX_HPP
#define HEX_HPP

namespace hex {

enum Instr {
  LDAM = 0x0,
  LDBM = 0x1,
  STAM = 0x2,
  LDAC = 0x3,
  LDBC = 0x4,
  LDAP = 0x5,
  LDAI = 0x6,
  LDBI = 0x7,
  STAI = 0x8,
  BR   = 0x9,
  BRZ  = 0xA,
  BRN  = 0xB,
  OPR  = 0xD,
  PFIX = 0xE,
  NFIX = 0xF,
};

enum OprInstr {
  BRB = 0x0,
  ADD = 0x1,
  SUB = 0x2,
  SVC = 0x3
};

enum class Syscall {
  EXIT  = 0,
  WRITE = 1,
  READ  = 2
};

const char *instrEnumToStr(Instr instr) {
  switch (instr) {
    case LDAM: return "LDAM";
    case LDBM: return "LDBM";
    case STAM: return "STAM";
    case LDAC: return "LDAC";
    case LDBC: return "LDBC";
    case LDAP: return "LDAP";
    case LDAI: return "LDAI";
    case LDBI: return "LDBI";
    case STAI: return "STAI";
    case BR:   return "BR";
    case BRZ:  return "BRZ";
    case BRN:  return "BRN";
    case PFIX: return "PFIX";
    case NFIX: return "NFIX";
    case OPR:  return "OPR";
    default:   return "UNKNOWN";
  }
}

const char *oprInstrEnumToStr(OprInstr oprInstr) {
  switch (oprInstr) {
    case BRB: return "BRB";
    case ADD: return "ADD";
    case SUB: return "SUB";
    case SVC: return "SVC";
    default:  return "UNKNOWN";
  }
}

const char *syscallEnumToStr(Syscall syscall) {
  switch (syscall) {
    case Syscall::EXIT:  return "EXIT";
    case Syscall::WRITE: return "WRITE";
    case Syscall::READ:  return "READ";
    default:             return "UNKNOWN";
  }
}

}; // End namespace hex.

#endif // HEX_HPP

