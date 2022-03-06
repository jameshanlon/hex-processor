#include "hex.hpp"

const char *hex::instrEnumToStr(Instr instr) {
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

const char *hex::oprInstrEnumToStr(OprInstr oprInstr) {
  switch (oprInstr) {
    case BRB: return "BRB";
    case ADD: return "ADD";
    case SUB: return "SUB";
    case SVC: return "SVC";
    default:  return "UNKNOWN";
  }
}

const char *hex::syscallEnumToStr(Syscall syscall) {
  switch (syscall) {
    case Syscall::EXIT:  return "EXIT";
    case Syscall::WRITE: return "WRITE";
    case Syscall::READ:  return "READ";
    default:             return "UNKNOWN";
  }
}
