module hex
  (
    input logic                i_clk,
    input logic                i_rst,
    // Syscall interface
    output logic               o_syscall_valid,
    output hex_pkg::syscall_t  o_syscall
  );

  // Memory instruction fetch interface
  logic             req_f_valid;
  hex_pkg::iaddr_t  req_f_addr;
  hex_pkg::instr_t  res_f_data;
  // Memory data read/write interface
  logic             req_d_valid;
  logic             req_d_we;
  hex_pkg::waddr_t  req_d_addr;
  hex_pkg::data_t   req_d_data;
  hex_pkg::data_t   res_d_data;

  processor u_processor (
    .i_rst           (i_rst),
    .i_clk           (i_clk),
    .o_f_valid       (req_f_valid),
    .o_f_addr        (req_f_addr),
    .i_f_data        (res_f_data),
    .o_d_valid       (req_d_valid),
    .o_d_we          (req_d_we),
    .o_d_addr        (req_d_addr),
    .o_d_data        (req_d_data),
    .i_d_data        (res_d_data),
    .o_syscall_valid (o_syscall_valid),
    .o_syscall       (o_syscall)
  );

  memory u_memory (
    .i_rst     (i_rst),
    .i_clk     (i_clk),
    .i_f_valid (req_f_valid),
    .i_f_addr  (req_f_addr),
    .o_f_data  (res_f_data),
    .i_d_valid (req_d_valid),
    .i_d_addr  (req_d_addr),
    .i_d_we    (req_d_we),
    .i_d_data  (req_d_data),
    .o_d_data  (res_d_data)
  );

  initial begin
    if ($test$plusargs("trace") != 0) begin
       $display("[%0t] Tracing to logs/vlt_dump.vcd...\n", $time);
       $dumpfile("logs/vlt_dump.vcd");
       $dumpvars();
    end
  end

endmodule
