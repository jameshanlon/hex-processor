![Build](https://github.com/jameshanlon/hex-processor/workflows/Build%20and%20test/badge.svg)

# Hex processor

This repository contains an assembler, simulator and hardware implementation
for the Hex processor architecture.

## Getting started

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
