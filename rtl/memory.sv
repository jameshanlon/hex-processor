// Single-port synchronous memory: one shared read/write port with a one-cycle
// read latency. The address is presented in cycle N and registered read data
// appears on o_rdata in cycle N+1. This models a single-port compiled SRAM hard
// macro, which can be substituted in directly.
//
// Instruction fetch and data access share this port; the processor's multi-cycle
// FSM guarantees they never occur in the same cycle. Memory is word addressed
// and returns whole 32-bit words; the processor extracts the fetched instruction
// byte from the returned word.
module memory
  (
    input  logic             i_clk,
    // Shared single read/write port.
    input  logic             i_valid,
    input  logic             i_we,
    input  hex_pkg::waddr_t  i_addr,
    input  hex_pkg::data_t   i_wdata,
    output hex_pkg::data_t   o_rdata
  );

  logic [hex_pkg::MEM_WIDTH-1:0] memory_q [hex_pkg::MEM_DEPTH-1:0] /* verilator public */;
  hex_pkg::data_t rdata_q;

  always_ff @(posedge i_clk)
    if (i_valid && i_we) begin
      memory_q[i_addr] <= i_wdata;
    end

  // Registered read: data presented one cycle after the address.
  always_ff @(posedge i_clk)
    if (i_valid && !i_we) begin
      rdata_q <= memory_q[i_addr];
    end

  assign o_rdata = rdata_q;

endmodule
