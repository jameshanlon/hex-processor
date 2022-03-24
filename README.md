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

## Building and running the tests

Clone and build Verilator and Yosys in repository.

```bash
$ mkdir Build
$ cd Build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug \
           -DVERILATOR_ROOT=<path-to-verilator-repo> \
           -DYOSYS_ROOT=<path-to-yosys-repo>
$ make
$ make test
```

Alternatively, the assembler and simulator can be built without Verilator
and/or Yosys:

```bash
$ cmake .. -DCMAKE_BUILD_TYPE=Debug \
           -DUSE_VERILATOR=NO \
           -DUSE_YOSYS=NO
```

## Running a program

Run the 'hello' assembly program:

```bash
$ export PATH=install/bin:$PATH
$ hexasm tests/asm/hello.S -o hello.bin
$ hexsim hello.bin
hello
```
