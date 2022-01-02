#ifndef HEX_HPP
#define HEX_HPP

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
  OPR,
  PFIX,
  NFIX,
};

enum OprInstr {
  ADD = 0,
  SUB,
  BRB,
  SVC
};

enum class Syscall {
  EXIT = 0,
  WRITE,
  READ
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

#endif // HEX_HPP

