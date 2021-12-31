module hextb
  (
    input logic  i_clk,
    input logic  i_rst,
    output logic o_result
  );

  assign o_result = 1;

  // Print some stuff as an example
  initial begin
    if ($test$plusargs("trace") != 0) begin
       $display("[%0t] Tracing to logs/vlt_dump.vcd...\n", $time);
       $dumpfile("logs/vlt_dump.vcd");
       $dumpvars();
    end
    $display("[%0t] Model running...\n", $time);
  end

endmodule
