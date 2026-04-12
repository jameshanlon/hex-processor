![Build](https://github.com/jameshanlon/hex-processor/workflows/Build%20and%20test/badge.svg)

# Hex processor

This repository contains an assembler, simulator and hardware implementation of
the Hex processor architecture, as well as a compiler for a simple programming
language 'X' that is targeted at it. The Hex architecture is designed to be
very simple and suitable for explaining how a computer works, whilst being
flexible enough to execute substantial programs, and easily extensible. With
complexities of modern computer architectures and compilers/tool chains being
difficult to penetrate, the intention of this project is to provide a simple
example Verilog implementation and supporting C++ tooling that can be used as
the basis for another project or just as a curiosity in itself.

## Architecture

The Hex processor has four 32-bit registers and uses 8-bit instructions
(4-bit opcode + 4-bit operand):

| Register | Description |
|----------|-------------|
| PC       | Program counter |
| AREG     | Accumulator (A register) |
| BREG     | B register |
| OREG     | Operand register |

### Instruction set

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| LDAM     | 0x0    | Load A from memory |
| LDBM     | 0x1    | Load B from memory |
| STAM     | 0x2    | Store A to memory |
| LDAC     | 0x3    | Load A with constant |
| LDBC     | 0x4    | Load B with constant |
| LDAP     | 0x5    | Load A with PC-relative address |
| LDAI     | 0x6    | Load A from indexed memory (B + operand) |
| LDBI     | 0x7    | Load B from indexed memory (B + operand) |
| STAI     | 0x8    | Store A to indexed memory (B + operand) |
| BR       | 0x9    | Branch (PC-relative) |
| BRZ      | 0xA    | Branch if A is zero |
| BRN      | 0xB    | Branch if A is negative |
| OPR      | 0xD    | Operate: BRB (0), ADD (1), SUB (2), SVC (3) |
| PFIX     | 0xE    | Prefix: shift operand left by 4 bits |
| NFIX     | 0xF    | Negate prefix: invert and shift operand |

Large immediate values are built up using PFIX/NFIX chains before the target
instruction.

## The X language

X is a simple imperative language with procedures, functions, and basic data
types. It compiles to Hex assembly. Example (Fibonacci):

```
val exit = 0;
val get = 2;

proc main() is
  exit(fib(get(0)))

func fib(val n) is
  if n = 0 then return 0
  else if n = 1 then return 1
  else return fib(n-1) + fib(n-2)
```

Key features:
- Procedures (`proc`) and functions (`func`) with `val`, `var`, and `array` parameters
- Higher-order procedure and function parameters
- Variables (`var`), constants (`val`), and arrays (`array`)
- Control flow: `if`/`then`/`else`, `while`/`do`, `{ ... }` sequences
- Syscalls: `0(code)` for exit, `1(char, stream)` for write, `2(stream)` for read

## Tools

| Tool     | Description |
|----------|-------------|
| `hexasm` | Assembler: compiles `.S` assembly files to `.bin` binaries |
| `hexdis` | Disassembler: disassembles `.bin` binaries back to readable assembly |
| `xcmp`   | Compiler: compiles `.x` X-language programs to `.bin` binaries |
| `hexsim` | Simulator: executes `.bin` binaries (use `-t` for instruction tracing) |
| `xrun`   | Runner: compiles and immediately executes an X program |
| `hextb`  | Verilator testbench: runs binaries on the RTL hardware model (requires Verilator) |

## Building and running the tests

### Dependencies

This project requires:
- CMake 3.14+
- C++20 compatible compiler
- Python 3
- Verilator (optional, for hardware simulation)

All other dependencies (fmt, Catch2) are fetched automatically via CMake FetchContent.

Ubuntu:
```bash
$ apt update && apt install build-essential cmake
```

macOS (with Homebrew):
```bash
$ brew install cmake
```

### Building

Configure and build with CMake:

```bash
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug \
           -DCMAKE_INSTALL_PREFIX=$(pwd)/install \
           -DUSE_VERILATOR=OFF
$ make -j8
$ make install
```

To build with Verilator for hardware simulation, replace `-DUSE_VERILATOR=OFF`
with `-DVERILATOR_ROOT=<path-to-verilator-repo>`.

### Testing

```bash
$ ctest --output-on-failure    # Run all tests
$ ./UnitTests                  # Run C++ unit tests only (Catch2)
$ ./UnitTests "<test name>"    # Run a single unit test
$ python3 ../tests/tests.py    # Run Python integration tests
```

Unit tests are in `tests/unit/` (assembly and X language features/programs).
Integration tests in `tests/tests.py` verify end-to-end compilation and execution.

## Running a program

Run the 'hello' assembly program:

```bash
$ export PATH=$(pwd)/install/bin:$PATH
$ hexasm tests/asm/hello.S -o hello.bin
$ hexsim hello.bin
hello
```

Run the 'hello' X program:

```bash
$ xcmp tests/x/hello_putval.x -o hello.bin
$ hexsim hello.bin
hello world
```

Disassemble a binary to see the instruction listing. If the binary contains
debug info (e.g. from `xcmp`), function labels are displayed:

```bash
$ hexdis hello.bin | head -10
  0x0000  97  BR   7
  0x0001  00  LDAM 0
  ...
  0x0008  51  LDAP 1
  0x0009  94  BR   4
  0x000a  11  LDBM 1
  0x000b  30  LDAC 0
  0x000c  82  STAI 2
  0x000d  d3  SVC

main:
```

Use `--no-labels` to suppress debug labels in the output.

Run with instruction tracing (`-t`) to see each cycle's execution. The trace
columns show: cycle number, PC address, symbol+offset, instruction, and the
operation performed:

```bash
$ hexsim hello.bin -t | head -5
0      0                   BR   7  pc = pc + oreg (7) (0x000008)
1      8                   LDAP 1  areg = pc (9) + oreg (1) 10
2      9                   BR   4  pc = pc + oreg (4) (0x00000e)
3      14     main+0       LDBM 1  breg = mem[oreg (0x000001)] (65536)
4      15     main+1       STAI 0  mem[breg (65536) + oreg (0) = 0x010000] = areg (10)
```

## Building the documentation

To build the Sphinx documentation:

```bash
$ virtualenv -p python3 env
$ source env/bin/activate
$ pip install -r docs/requirements.txt
$ cmake .. -DBUILD_DOCS=ON
$ make
```
