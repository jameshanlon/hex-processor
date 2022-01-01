module hex
  (
    input logic  i_clk,
    input logic  i_rst
  );

  // Memory instruction fetch interface
  logic            req_f_valid;
  hex_pkg::addr_t  req_f_addr;
  hex_pkg::instr_t res_f_data;
  // Memory data read/write interface
  logic            req_d_valid;
  logic            req_d_we;
  hex_pkg::addr_t  req_d_addr;
  hex_pkg::data_t  req_d_data;
  hex_pkg::data_t  res_d_data;

  processor u_processor (
    .i_rst     (i_rst),
    .i_clk     (i_clk),
    .o_f_valid (req_f_valid),
    .o_f_addr  (req_f_addr),
    .i_f_data  (res_f_data),
    .o_d_valid (req_d_valid),
    .o_d_we    (req_d_we),
    .o_d_addr  (req_d_addr),
    .o_d_data  (req_d_data),
    .i_d_data  (res_d_data)
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

  //reg [1000:0] filename;
  //integer file, position, c, size;

  initial begin
    if ($test$plusargs("trace") != 0) begin
       $display("[%0t] Tracing to logs/vlt_dump.vcd...\n", $time);
       $dumpfile("logs/vlt_dump.vcd");
       $dumpvars();
    end
    //if ($value$plusargs("binary=%s", filename)) begin
    //  // Open the file
    //  $display("Loading binary file %0s", filename);
    //  file = $fopen(filename, "rb");
    //  // Determine the size
    //  $fseek(file, -1, 2);
    //  size = $ftell(file);
    //  $rewind(file);
    //  $display("%0d bytes", size);
    //  // Read the data
    //  //c = $fgetc(file);
    //  while (/*c != -1*/!$feof(file)) begin
    //    c = $fgetc(file);
    //    $display(".");
    //  end
    //  $fclose(file);
    //end
    $display("[%0t] Model running...\n", $time);
  end

endmodule
