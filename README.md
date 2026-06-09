![Build](https://github.com/jameshanlon/hex-processor/workflows/Build%20and%20test/badge.svg)

# Hex processor

This repository contains an assembler, disassembler, simulator and hardware
implementation of the **Hex processor architecture**, as well as a compiler for
a simple parallel programming language 'X' that is targeted at it.

The Hex architecture is designed to be simple enough for explaining how a
computer works, whilst being flexible enough to execute substantial programs,
and easily extensible. With complexities of modern computer architectures and
compilers/tool chains being difficult to penetrate, the intention of this
project is to provide a simple example Verilog implementation and supporting C++
tooling that can be used as the basis for another project or just as a curiosity
in itself.

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
| OPR      | 0xD    | Operate (see sub-operations below) |
| PFIX     | 0xE    | Prefix: shift operand left by 4 bits |
| NFIX     | 0xF    | Negate prefix: invert and shift operand |

Large immediate values are built up using PFIX/NFIX chains before the target
instruction.

The `OPR` opcode selects a register-to-register sub-operation via its operand:

| Sub-op | Operand | Description |
|--------|---------|-------------|
| BRB    | 0x0     | Branch to address in B register |
| ADD    | 0x1     | A = A + B |
| SUB    | 0x2     | A = A - B |
| SVC    | 0x3     | Supervisor call (syscall, see below) |
| IN     | 0x4     | Receive a word from channel slot B into A (blocking) |
| OUT    | 0x5     | Send word A to channel slot B (blocking) |

### Channels

`IN` and `OUT` perform a synchronous rendezvous over a point-to-point channel:
the B register selects one of the processor's four link slots and the A register
carries the value (received for `IN`, sent for `OUT`). The first party to arrive
blocks until its partner is ready, at which point the value is exchanged and both
continue. A core can be wired to up to four channels; operating on an unwired
slot is a runtime error.

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
- Concurrency and message passing: `par`, `chan` declarations and formals, and
  the `!` (send) and `?` (receive) channel operators

### Concurrency

A `par` block runs its branches on separate processors, connected by the
channels passed to them. Each branch communicates only by sending (`!`) and
receiving (`?`) over channels — there is no shared memory. When `main` ends in a
top-level `par`, the compiler emits a *network container* describing the per-core
images and how their channel slots are wired together (see Tools below).

Example — a three-stage pipeline (`source -> relay -> sink`) on three
processors connected by two channels (`examples/pipe.x`):

```
val put = 1;

proc putval(val c) is put(c, 0)

proc source(chan out) is out ! 'P'

proc relay(chan in, chan out) is
  var v;
  { in ? v; out ! v }

proc sink(chan in) is
  var v;
  { in ? v; putval(v) }

proc main() is
  chan a;
  chan b;
  par { source(a); relay(a, b); sink(b) }
```

### Example programs

The `examples/` directory contains runnable example programs. Sequential
examples include `gcd.x`, `collatz.x`, `ackermann.x`, `primes.x`, `binsearch.x`,
`reverse.x` and `hanoi.x` (alongside `fib.x`, `fac.x`, `bubblesort.x`, etc.).
Concurrent examples (each a network of up to four cores) include:

| Program | What it shows |
|---------|---------------|
| `pipe.x` | Three-stage pipeline (`source -> relay -> sink`) |
| `pingpong.x` | Two cores exchanging a value |
| `ring.x` | Token ring reusing one process across cores |
| `buffer.x` | Streaming a sequence through a buffer with a sentinel |
| `sieve.x` | Concurrent prime sieve (pipeline of filters) |
| `farm.x` | Worker farm: distributor → workers → collector (fan-out/fan-in) |
| `reduce.x` | Parallel sum over a binary tree of cores |
| `stencil.x` | 1D nearest-neighbour halo exchange (bidirectional channels) |

## Tools

| Tool     | Description |
|----------|-------------|
| `hexasm` | Assembler: compiles `.S` assembly files to `.bin` binaries |
| `hexdis` | Disassembler: disassembles `.bin` binaries back to readable assembly |
| `xcmp`   | Compiler: compiles `.x` X-language programs to `.bin` binaries |
| `hexsim` | Simulator: executes a single image or a multi-core network container (use `-t` for instruction tracing) |
| `xrun`   | Runner: compiles and immediately executes an X program |
| `hextb`  | Verilator testbench: runs a single image or network container on the RTL multi-core network (requires Verilator) |

A plain `.bin` holds one processor image. A program whose `main` is a `par`
block instead produces a *network container* holding one image per core plus the
channel wiring between their link slots; `hexsim` and `hextb` detect the magic
and boot the whole network. `hexsim` reports the exit code of the first
processor to halt and detects deadlock when every core is blocked on a channel.

## Repository layout

```
src/      Library code: header-only implementations (*.hpp) plus hex.cpp
tools/    CLI front-ends, one .cpp per executable (hexasm, hexdis, hexsim,
          xcmp, xrun, hextb)
rtl/      SystemVerilog implementation (processor core, memory, link
          interface, router and multi-core network top)
examples/ Runnable X example programs (*.x)
tests/    Unit tests (tests/unit/, Catch2), integration tests (tests/tests.py),
          and assembly test programs (tests/asm/*.S)
docs/     Sphinx documentation
cmake/    CMake helper modules
```

## Building and running the tests

### Dependencies

This project requires:
- CMake 3.20+
- C++20 compatible compiler
- Python 3
- Verilator 5.0+ (optional, for hardware simulation; built automatically if not
  installed)

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

To build with Verilator for hardware simulation, drop `-DUSE_VERILATOR=OFF` (it
defaults to `ON`). A suitable system Verilator (>= 5.0) is used if found —
honouring `-DVERILATOR_ROOT=<path>` — otherwise a pinned version is fetched and
built automatically.

### Testing

```bash
$ ctest --output-on-failure    # Run all tests
$ ./UnitTests                  # Run C++ unit tests only (Catch2)
$ ./UnitTests "<test name>"    # Run a single unit test
$ python3 ../tests/tests.py    # Run Python integration tests
```

Unit tests are in `tests/unit/`, covering the assembler, disassembler, X
language features/programs, and the multi-core simulator (channels and
message-passing networks). Integration tests in `tests/tests.py` verify
end-to-end compilation and execution.

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
$ xcmp examples/hello_putval.x -o hello.bin
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
