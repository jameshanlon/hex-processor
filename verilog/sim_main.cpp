#include <verilated.h>

#include "Vhextb.h"

double sc_time_stamp() { return 0; }

int main(int argc, char** argv, char **env) {

  // Prevent unused variable warnings
  if (false && argc && argv && env) {}

  Verilated::mkdir("logs");
  const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};

  contextp->debug(0);
  contextp->randReset(2);
  contextp->traceEverOn(true);
  contextp->commandArgs(argc, argv);

  const std::unique_ptr<Vhextb> top{new Vhextb{contextp.get(), "TOP"}};

  // Set input signals
  top->i_rst = 0;
  top->i_clk = 0;

  while (!contextp->gotFinish()) {
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
    // Report state.
    VL_PRINTF("[%" VL_PRI64 "d] clk=%x rst=%x\n",
              contextp->time(), top->i_clk, top->i_rst);
  }

  top->final();
  return 0;
}
