#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <exception>
#include <boost/format.hpp>
#include <verilated.h>

#include "Vhex_pkg.h"
#include "Vhex_pkg_hex.h"
#include "Vhex_pkg_memory.h"
#include "Vhex_pkg_processor.h"
#include "hex.hpp"
#include "hexsimio.hpp"

double sc_time_stamp() { return 0; }

constexpr size_t RESET_BEGIN = 1;
constexpr size_t RESET_END = 10;

hex::HexSimIO io(std::cin, std::cout);

void load(const char *filename,
          const std::unique_ptr<Vhex_pkg> &top) {

  // Load the binary file.
  std::streampos fileSize;
  std::ifstream file(filename, std::ios::binary);

  // Get length of file.
  file.seekg(0, std::ios::end);
  fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  // Check the file length matches.
  unsigned remainingFileSize = static_cast<unsigned>(fileSize) - 4;
  remainingFileSize = (remainingFileSize + 3U) & ~3U; // Round up to multiple of 4.
  unsigned programSize;
  file.read(reinterpret_cast<char*>(&programSize), 4);
  programSize <<= 2;
  if (programSize != remainingFileSize) {
    std::cerr << boost::format("Warning: mismatching program size %d != %d\n")
                   % programSize % remainingFileSize;
  }

  // Read the file contents.
  std::vector<uint32_t> buffer(remainingFileSize);
  file.read(reinterpret_cast<char*>(buffer.data()), remainingFileSize);

  // Write program to DUT memory.
  std::memcpy(top->hex->u_memory->memory_q.data(), buffer.data(), buffer.size());

  std::cout << "Wrote " << programSize << " bytes to memory\n";
}

void handleSyscall(hex::Syscall syscall,
                   const std::unique_ptr<Vhex_pkg> &top,
                   bool trace) {
  unsigned spWordIndex = top->hex->u_memory->memory_q[1];
  switch (syscall) {
    case hex::Syscall::EXIT:
      if (trace) {
        std::cout << "exit\n";
      }
      break;
    case hex::Syscall::WRITE: {
      char value = top->hex->u_memory->memory_q[spWordIndex+2];
      int stream = top->hex->u_memory->memory_q[spWordIndex+3];
      if (trace) {
        std::cout << boost::format("output(%c, %d)\n") % value % stream;
      }
      io.output(value, stream);
      break;
    }
    case hex::Syscall::READ: {
      int stream = top->hex->u_memory->memory_q[spWordIndex+2];
      if (trace) {
        std::cout << boost::format("input(%d)\n") % stream;
      }
      top->hex->u_memory->memory_q[spWordIndex+1] = io.input(stream) & 0xFF;
      break;
    }
    default:
      throw std::runtime_error("invalid syscall");
  }
}

void run(const std::unique_ptr<VerilatedContext> &contextp,
         const std::unique_ptr<Vhex_pkg> &top,
         bool trace,
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
    if (top->i_clk) {
      if (contextp->time() > RESET_BEGIN && contextp->time() < RESET_END) {
        top->i_rst = 1; // Assert reset
      } else {
        top->i_rst = 0; // Deassert reset
      }
    }
    // Evaluate the design.
    top->eval();
    if (top->i_clk) {
      cycle_count++;
    }
    // Trace
    if (trace && top->i_clk && contextp->time() > RESET_END) {
      auto instr = instrEnumToStr(static_cast<hex::Instr>((top->hex->u_processor->instr >> 4) & 0xF));
      std::cout << boost::format("[%-6d] %-6d 0x%02x %-6s\n")
                     % contextp->time()
                     % top->hex->u_processor->pc_q
                     % static_cast<unsigned>(top->hex->u_processor->instr)
                     % instr;
    }
    // Handle syscalls
    if (top->i_clk && top->o_syscall_valid) {
      auto syscall = static_cast<hex::Syscall>(top->o_syscall);
      if (syscall == hex::Syscall::EXIT) {
        break;
      }
      handleSyscall(syscall, top, trace);
    }
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
  std::cout << "  -t,--trace      Enable instruction tracing\n";
  std::cout << "  --max-cycles N  Limit the number of simulation cycles (default: 0)\n";
}

int main(int argc, const char** argv) {
  try {
    // Handle arguments.
    const char *filename = nullptr;
    bool trace = false;
    size_t maxCycles = 0;
    for (unsigned i = 1; i < argc; ++i) {
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
    run(contextp, top, trace, maxCycles);
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
