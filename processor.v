`define LDAM 4`h0
`define LDBM 4`h1
`define STAM 4`h2
`define LDAC 4`h3
`define LDBC 4`h4
`define LDAP 4`h5
`define LDAI 4`h6
`define LDBI 4`h7
`define STAI 4`h8

`define BR   4`h9
`define BRZ  4`hA
`define BRN  4`hB

`define OPR  4`hD
`define PFIX 4`hE
`define NFIX 4`hF

`define BRB  4`h0
`define ADD  4`h1
`define SUB  4`h2
`define SVC  4`h3

module processor();

  input wire clk;
  input wire rst;

  reg [7:0]  mem[0:4096];
  reg [31:0] pc;
  reg [31:0] areg;
  reg [31:0] breg;
  reg [31:0] oreg;
  reg [8:0]  inst;
  reg [4:0]  opcode;

  task load(input [31:0] addr,
            output [31:0] data);
    begin
      data = { mem[addr + 3],
               mem[addr + 2],
               mem[addr + 1],
               mem[addr + 0] };
    end
  endtask

  task store(input [31:0] addr,
             output [31:0] data);
    begin
      { mem[addr + 3],
        mem[addr + 2],
        mem[addr + 1],
        mem[addr + 0] } = data;
    end
  endtask

  always @(posedge clk)
  begin

    // Fetch instruction.
    load(pc, inst);

    // Decode instruction.
    opcode = inst[8:4];
    oreg = oreg | inst[3:0];

    // Increment PC.
    pc = pc + 1;

    // Execute instruction.
    case (inst_opcode)
    `LDAM:
      begin
        load(oreg, areg);
        oreg = 0;
      end
    `LDBM:
      begin
        load(oreg, breg);
        oreg = 0;
      end
    `STAM:
      begin
        store(oreg, areg);
        oreg = 0;
      end
    `LDAC:
      begin
        areg = oreg;
        oreg = 0;
      end
    `LDBC:
      begin
        breg = oreg;
        oreg = 0;
      end
    `LDAP:
      begin
        areg = pc + oreg;
        oreg = 0;
      end
    `LDAI:
      begin
        reg[31:0] addr = areg + oreg;
        load(addr, areg);
        oreg = 0;
      end
    `LDBI:
      begin
        reg[31:0] addr = breg + oreg;
        load(addr, breg);
        oreg = 0;
      end
    `STAI:
      begin
        reg[31:0] addr = breg + oreg;
        store(addr, areg);
      end
    `BR:
      begin
        pc = pc + oreg;
        oreg = 0;
      end
    `BRZ:
      begin
        if (areg == 0)
          pc = pc + oreg;
        oreg = 0;
      end
    `BRN:
      begin
        if (areg[31] == 1)
          pc = pc + oreg;
        oreg = 0;
      end
    `PFIX:
      oreg = oreg << 4;
    `NFIX:
      oreg = 0`hFFFFFF00 | (oreg << 4);
    `OPR:
      begin
        case (oreg)
        `BRB:
          pc = breg;
        `ADD:
          areg = areg + breg;
        `SUB:
          areg = areg - breg;
        `SVC:
          // TODO
        endcase
       oreg = 0;
      end
    endcase

  end

endmodule

module main();

  processor p();

endmodule
