module processor();

  input wire clk;
  input wire rst;

  reg [7:0]  mem[0:4096];
  reg [31:0] gpr[0:31];
  reg [31:0] pc;
  reg [31:0] ins;

  always @(posedge clk)
  begin

  end

endmodule

module main();

  processor p();

endmodule
