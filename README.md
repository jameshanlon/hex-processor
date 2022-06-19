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

To build with the documentation, first create a virtualenv with the requirements:

```bash
$ virtualenv -p python3 env
...
$ source env/bin/activate
$ pip install -r docs/requirements.txt
```

Ubuntu requirements:

```bash
apt update && apt install build-essential cmake libboost-all-dev
```

Then, configure and run a build with CMake:

```bash
$ mkdir Build
$ cd Build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug \
           -DVERILATOR_ROOT=<path-to-verilator-repo> \
           -DYOSYS_ROOT=<path-to-yosys-repo> \
           -DBUILD_DOCS=OFF
$ make
$ make test # Run all the unit tests.
```

Alternatively, the Verilator and/or Yosys components of the build can be
excluded if these tools are not available:

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

Run the 'hello' X program:

```bash
$ export PATH=install/bin:$PATH
$ xcmp tests/x/hello.x -o hello.bin
$ hexsim hello.bin
hello
$ hexsim hello.bin -t # Run with instruction tracing
0      0                   BR   7  pc = pc + oreg (7) (0x000008)
1      8                   LDAP 1  areg = pc (9) + oreg (1) 10
2      9                   BR   4  pc = pc + oreg (4) (0x00000e)
3      14     main+0       LDBM 1  breg = mem[oreg (0x000001)] (65536)
4      15     main+1       STAI 0  mem[breg (65536) + oreg (0) = 0x010000] = areg (10)
5      16     main+2       NFIX 15 oreg = 0xFFFFFF00 | oreg (15) << 4 (0xfffffff0)
6      17     main+3       LDAC 14 areg = oreg 4294967294
7      18     main+4       OPR  1  ADD areg = areg (4294967294) + breg (65536) (65534)
8      19     main+5       STAM 1  mem[oreg (0x000001)] = areg 65534
9      20     main+6       PFIX 6  oreg = oreg (6) << 4 (0x000060)
10     21     main+7       LDAC 8  areg = oreg 104
...
```
