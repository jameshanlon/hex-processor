#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fmt/format.h>
#include <iostream>
#include <vector>
#include <verilated.h>

#include "Vntb.h"
#include "Vntb_core.h"
#include "Vntb_memory.h"
#include "Vntb_network_top.h"
#include "Vntb_processor.h"
#include "hex.hpp"
#include "hexcontainer.hpp"
#include "hexsimio.hpp"

double sc_time_stamp() { return 0; }

hex::HexSimIO io(std::cin, std::cout);

// Must match hex_pkg::NUM_CORES in the SystemVerilog.
constexpr unsigned NUM_CORES = 4;
static_assert(NUM_CORES == 4,
              "coreOf() enumerates exactly 4 cores by their generate-loop "
              "names; extend it if NUM_CORES changes.");

// A two-instruction loop (NFIX F; BR E) that spins in place forever (BR E adds
// -2, returning to the NFIX) without any syscall or channel op, keeping cores
// with no image quiescent.
constexpr uint32_t HALT_LOOP = 0x00009EFFu;

// Reach core k. network_top instantiates the cores in a generate loop, so
// Verilator exposes them under mangled names (g_core[k].u_core); the names are
// stable because the Verilator version is pinned.
static Vntb_core *coreOf(const std::unique_ptr<Vntb> &top, unsigned k) {
  switch (k) {
  case 0: return top->network_top->g_core__BRA__0__KET____DOT__u_core;
  case 1: return top->network_top->g_core__BRA__1__KET____DOT__u_core;
  case 2: return top->network_top->g_core__BRA__2__KET____DOT__u_core;
  case 3: return top->network_top->g_core__BRA__3__KET____DOT__u_core;
  default: throw std::runtime_error("core index out of range");
  }
}

static IData *memOf(const std::unique_ptr<Vntb> &top, unsigned k) {
  return coreOf(top, k)->u_memory->memory_q.data();
}

// Service one core's syscall, reading arguments from its own memory.
static void handleSyscall(unsigned k, hex::Syscall syscall,
                          const std::unique_ptr<Vntb> &top, int &exitValue,
                          bool &exited) {
  IData *mem = memOf(top, k);
  unsigned sp = mem[1];
  switch (syscall) {
  case hex::Syscall::EXIT:
    exitValue = mem[sp + 2];
    exited = true;
    break;
  case hex::Syscall::WRITE:
    io.output(static_cast<char>(mem[sp + 2]), static_cast<int>(mem[sp + 3]));
    break;
  case hex::Syscall::READ:
    mem[sp + 1] = io.input(mem[sp + 2]) & 0xFF;
    break;
  default:
    throw std::runtime_error("invalid syscall");
  }
}

// Read the container, fill every core with the halt loop, and load the images
// into the active cores. Returns the parsed container (its edges are used to
// program the route tables).
static hexcontainer::Container load(const char *filename,
                                    const std::unique_ptr<Vntb> &top) {
  auto container = hexcontainer::read(filename);
  unsigned numActive = container.images.size();
  if (numActive > NUM_CORES) {
    throw std::runtime_error("container has more processors than NUM_CORES");
  }

  // Quiescent default for every core.
  for (unsigned k = 0; k < NUM_CORES; k++) {
    memOf(top, k)[0] = HALT_LOOP;
  }

  // Load each image's code into its core's memory.
  for (unsigned i = 0; i < numActive; i++) {
    auto &image = container.images[i];
    uint32_t programSizeWords;
    std::memcpy(&programSizeWords, image.data(), 4);
    std::memcpy(memOf(top, i), image.data() + 4, programSizeWords << 2);
  }
  std::cout << fmt::format("Loaded {} processor image(s)\n", numActive);
  return container;
}

// Program one route-table entry: core->slot maps to (dstCore, dstSlot). Driven
// over one clock with the config write enable asserted.
static void writeRoute(const std::unique_ptr<VerilatedContext> &ctx,
                       const std::unique_ptr<Vntb> &top, unsigned core,
                       unsigned slot, unsigned dstCore, unsigned dstSlot) {
  top->i_cfg_we = 1;
  top->i_cfg_core = core;
  top->i_cfg_slot = slot;
  top->i_cfg_dst_core = dstCore;
  top->i_cfg_dst_slot = dstSlot;
  ctx->timeInc(1); top->i_clk = 0; top->eval();
  ctx->timeInc(1); top->i_clk = 1; top->eval();
  top->i_cfg_we = 0;
}

int run(const std::unique_ptr<VerilatedContext> &ctx,
        const std::unique_ptr<Vntb> &top, const char *filename, bool trace,
        size_t maxCycles) {
  top->i_clk = 0;
  top->i_cfg_we = 0;
  top->i_rst = 1;
  top->eval();

  auto container = load(filename, top);
  unsigned numActive = container.images.size();

  // Hold reset for a few cycles, then program the routing tables (config writes
  // are independent of reset).
  for (int i = 0; i < 4; i++) {
    ctx->timeInc(1); top->i_clk = 0; top->eval();
    ctx->timeInc(1); top->i_clk = 1; top->eval();
  }
  for (auto &e : container.edges) {
    writeRoute(ctx, top, e.procA, e.slotA, e.procB, e.slotB);
    writeRoute(ctx, top, e.procB, e.slotB, e.procA, e.slotA);
  }
  top->i_rst = 0;

  std::vector<bool> exited(numActive, false);
  std::vector<unsigned> prevPc(numActive, 0xFFFFFFFFu);
  unsigned numExited = 0;
  int exitCode = 0;
  bool haveExit = false;
  uint64_t cycles = 0;
  // A channel rendezvous legitimately freezes the participating cores' PCs for
  // a few cycles while the flit/ACK traverse the routers. Only declare deadlock
  // after a sustained stretch with no PC change and no syscall anywhere.
  unsigned noProgress = 0;
  constexpr unsigned DEADLOCK_THRESHOLD = 256;

  while (numExited < numActive && (maxCycles == 0 || cycles <= maxCycles)) {
    // Falling then rising edge.
    ctx->timeInc(1); top->i_clk = 0; top->eval();
    ctx->timeInc(1); top->i_clk = 1; top->eval();
    cycles++;

    bool progressed = false;
    for (unsigned k = 0; k < numActive; k++) {
      if (exited[k]) {
        continue;
      }
      // Syscalls.
      if ((top->o_syscall_valid >> k) & 1) {
        bool justExited = false;
        int exitValue = 0;
        handleSyscall(k, static_cast<hex::Syscall>(top->o_syscall[k]), top,
                      exitValue, justExited);
        progressed = true;
        if (justExited) {
          exited[k] = true;
          numExited++;
          if (!haveExit) {
            exitCode = exitValue; // first EXIT sets the system exit code
            haveExit = true;
          }
        }
      }
      // Progress detection for deadlock.
      unsigned pc = coreOf(top, k)->u_processor->pc_q;
      if (pc != prevPc[k]) {
        progressed = true;
      }
      prevPc[k] = pc;
      if (trace) {
        std::cout << fmt::format("[{:6}] core{} pc={}\n", cycles, k, pc);
      }
    }

    noProgress = progressed ? 0 : noProgress + 1;
    if (noProgress > DEADLOCK_THRESHOLD && numExited < numActive) {
      std::string msg = "deadlock: cores blocked:";
      for (unsigned k = 0; k < numActive; k++) {
        if (!exited[k]) {
          msg += fmt::format(" {}", k);
        }
      }
      top->final();
      throw std::runtime_error(msg);
    }
  }

  top->final();
  return exitCode;
}

static void help(const char **argv) {
  std::cout << "Hex multi-core processor testbench\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
  std::cout << "  file            A binary or network container to execute\n";
  std::cout << "  -h,--help       Display this message\n";
  std::cout << "  -t,--trace      Enable per-core PC tracing\n";
  std::cout << "  --max-cycles N  Limit simulation cycles (default: 0)\n";
}

int main(int argc, const char **argv) {
  try {
    const char *filename = nullptr;
    bool trace = false;
    size_t maxCycles = 0;
    for (int i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        return 1;
      } else if (std::strcmp(argv[i], "-t") == 0 ||
                 std::strcmp(argv[i], "--trace") == 0) {
        trace = true;
      } else if (std::strcmp(argv[i], "--max-cycles") == 0) {
        maxCycles = std::stoull(argv[++i]);
      } else if (argv[i][0] == '+') {
        continue;
      } else if (!filename) {
        filename = argv[i];
      } else {
        throw std::runtime_error("cannot specify more than one file");
      }
    }
    if (!filename) {
      help(argv);
      return 1;
    }
    const std::unique_ptr<VerilatedContext> ctx{new VerilatedContext};
    ctx->commandArgs(argc, argv);
    const std::unique_ptr<Vntb> top{new Vntb{ctx.get(), "TOP"}};
    return run(ctx, top, filename, trace, maxCycles);
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
