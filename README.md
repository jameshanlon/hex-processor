# Hex processor

This repository contains an assembler, simulator and hardware implementation
for the Hex processor architecture.

## Getting started

```bash
$ mkdir Build
$ mkdir Install
$ cd Build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug \
           -DVERILATOR_ROOT=<path-to-verilator-repo> \
           -DYOSYS_ROOT=<path-to-yosys-repo>
$ make
$ make test
```
