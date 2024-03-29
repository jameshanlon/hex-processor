#ifndef HEX_HPP
#define HEX_HPP

//===---------------------------------------------------------------------===//
// Hex enumeration definitions and conversion functions.
//===---------------------------------------------------------------------===//

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
  READ  = 2,
  NUM_VALUES
};

const char *instrEnumToStr(Instr instr);
const char *oprInstrEnumToStr(OprInstr oprInstr);
const char *syscallEnumToStr(Syscall syscall);

const int MAX_MEMORY_SIZE_WORDS = 200000;

} // End namespace hex.

#endif // HEX_HPP
