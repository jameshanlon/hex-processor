#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <exception>
#include <verilated.h>

#include "Vhex_pkg.h"
#include "Vhex_pkg_hex.h"
#include "Vhex_pkg_memory.h"

double sc_time_stamp() { return 0; }

void load(const char *filename,
          const std::unique_ptr<Vhex_pkg> &top) {
  // Load the binary file.
  std::streampos fileSize;
  std::ifstream file(filename, std::ios::binary);

  // Get length of file.
  file.seekg(0, std::ios::end);
  fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  // Read the file contents.
  std::vector<uint32_t> buffer(fileSize);
  file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

  // Write program to DUT memory.
  std::memcpy(top->hex->u_memory->memory_q.data(), buffer.data(), buffer.size());

  std::cout << "Wrote " << fileSize << " bytes to memory\n";
}

void run(const std::unique_ptr<VerilatedContext> &contextp,
         const std::unique_ptr<Vhex_pkg> &top,
         size_t maxCycles) {
  uint64_t cycle_count = 0;

  // Set input signals
  top->i_rst = 0;
  top->i_clk = 0;

  while (!contextp->gotFinish() &&
         (maxCycles > 0 ? cycle_count < maxCycles : true)) {
    contextp->timeInc(1);
    // Toggle the clock.
    top->i_clk = !top->i_clk;
    // Assert reset initially.
    if (!top->i_clk) {
      if (contextp->time() > 1 && contextp->time() < 10) {
        top->i_rst = !0; // Assert reset
      } else {
        top->i_rst = !1; // Deassert reset
      }
    }
    // Evaluate the design.
    top->eval();
    cycle_count++;
    // Report state.
    VL_PRINTF("[%lld] clk=%x rst=%x\n",
              contextp->time(), top->i_clk, top->i_rst);
  }

  top->final();
}

static void help(const char **argv) {
  std::cout << "Hex processor testbench\n\n";
  std::cout << "Usage: " << argv[0] << " file\n\n";
  std::cout << "Positional arguments:\n";
  std::cout << "  file A binary file to execute\n\n";
  std::cout << "Optional arguments:\n";
  std::cout << "  -h,--help       Display this message\n";
  std::cout << "  --max-cycles N  Limit the number of simulation cycles (default: 0)\n";
}

int main(int argc, const char** argv) {
  try {
    // Handle arguments.
    const char *filename = nullptr;
    size_t maxCycles = 0;
    for (unsigned i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "-h") == 0 ||
          std::strcmp(argv[i], "--help") == 0) {
        help(argv);
        return 1;
      } else if (std::strcmp(argv[i], "--max-cycles") == 0) {
        maxCycles = std::stoull(argv[++i]);
      } else if (argv[i][0] == '+') {
        // Skip plusargs.
        continue;
      } else {
        // Positional argument.
        if (!filename) {
          filename = argv[i];
        } else {
          throw std::runtime_error("cannot specify more than one file");
        }
      }
    }
    // Setup TB and DUT.
    Verilated::mkdir("logs");
    const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};
    contextp->debug(0);
    contextp->randReset(2);
    contextp->traceEverOn(true);
    contextp->commandArgs(argc, argv);
    const std::unique_ptr<Vhex_pkg> top{new Vhex_pkg{contextp.get(), "TOP"}};
    // Run.
    load(filename, top);
    run(contextp, top, maxCycles);
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
