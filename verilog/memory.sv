module memory
  (
    input  logic            i_rst,
    input  logic            i_clk,
    // Read only instruction fetch port.
    input  logic            i_f_valid,
    input  hex_pkg::addr_t  i_f_addr,
    output hex_pkg::instr_t o_f_data,
    // Read/write data port.
    input  logic            i_d_valid,
    input  logic            i_d_we,
    input  hex_pkg::addr_t  i_d_addr,
    input  hex_pkg::data_t  i_d_data,
    output hex_pkg::data_t  o_d_data
  );

  logic [hex_pkg::MEM_WIDTH-1:0] memory_q [hex_pkg::MEM_DEPTH-1:0] /* verilator public */;

  always_ff @(posedge i_clk or posedge i_rst)
    if (i_d_valid && i_d_we) begin
      memory_q[i_d_addr[hex_pkg::MEM_ADDR_WIDTH-1:2]] <= i_d_data;
    end

  logic [4:0] fetch_byte_addr; // Byte index in a 32-bit word.
  assign fetch_byte_addr = {3'b000, i_f_addr[1:0]} << 3;

  assign o_f_data = memory_q[i_f_addr[hex_pkg::MEM_ADDR_WIDTH-1:2]][fetch_byte_addr +: 8];
  assign o_d_data = memory_q[i_d_addr[hex_pkg::MEM_ADDR_WIDTH-1:2]];

endmodule
