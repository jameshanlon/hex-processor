module hex
  (
    input logic                i_clk,
    input logic                i_rst,
    // Syscall interface
    output logic               o_syscall_valid,
    output hex_pkg::syscall_t  o_syscall
  );

  // Shared single memory port.
  logic             mem_valid;
  logic             mem_we;
  hex_pkg::waddr_t  mem_addr;
  hex_pkg::data_t   mem_wdata;
  hex_pkg::data_t   mem_rdata;

  processor u_processor (
    .i_rst           (i_rst),
    .i_clk           (i_clk),
    .o_mem_valid     (mem_valid),
    .o_mem_we        (mem_we),
    .o_mem_addr      (mem_addr),
    .o_mem_wdata     (mem_wdata),
    .i_mem_rdata     (mem_rdata),
    .o_syscall_valid (o_syscall_valid),
    .o_syscall       (o_syscall),
    // No channels in the single-core top: never stall, no received word.
    .o_op_out        (),
    .o_op_in         (),
    .o_chan_slot     (),
    .o_chan_areg     (),
    .i_liu_busy      (1'b0),
    .i_liu_in_word   ('0)
  );

  memory u_memory (
    .i_clk   (i_clk),
    .i_valid (mem_valid),
    .i_we    (mem_we),
    .i_addr  (mem_addr),
    .i_wdata (mem_wdata),
    .o_rdata (mem_rdata)
  );

  initial begin
    if ($test$plusargs("trace") != 0) begin
       $display("[%0t] Tracing to logs/vlt_dump.vcd...\n", $time);
       $dumpfile("logs/vlt_dump.vcd");
       $dumpvars();
    end
  end

endmodule
