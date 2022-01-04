module processor
  (
    input logic               i_rst,
    input logic               i_clk,
    // Fetch memory port
    output logic              o_f_valid,
    output hex_pkg::iaddr_t   o_f_addr,
    input  hex_pkg::instr_t   i_f_data,
    // Data memory port
    output logic              o_d_valid,
    output logic              o_d_we,
    output hex_pkg::waddr_t   o_d_addr,
    output hex_pkg::data_t    o_d_data,
    input  hex_pkg::data_t    i_d_data,
    // Syscall interface
    output logic              o_syscall_valid,
    output hex_pkg::syscall_t o_syscall
  );

  // State
  hex_pkg::iaddr_t   pc_q /* verilator public */;
  hex_pkg::data_t    areg_q;
  hex_pkg::data_t    breg_q;
  hex_pkg::data_t    oreg_q;

  // Nets
  hex_pkg::instr_t   instr /* verilator public */;
  logic              instr_svc;
  hex_pkg::iaddr_t   pc_d;
  hex_pkg::data_t    areg_d;
  hex_pkg::data_t    breg_d;
  hex_pkg::data_t    oreg_d;
  hex_pkg::data_t    opr_d;
  hex_pkg::iaddr_t   opr_iaddr;
  hex_pkg::opcode_t  instr_opc;
  hex_pkg::operand_t instr_opr;

  always_ff @(posedge i_clk or posedge i_rst)
    if (i_rst) begin
      pc_q   <= '0;
      areg_q <= '0;
      breg_q <= '0;
      oreg_q <= '0;
    end else begin
      pc_q   <= pc_d;
      areg_q <= areg_d;
      breg_q <= breg_d;
      oreg_q <= oreg_d;
    end

  // Instruction fetch.
  assign o_f_valid = 1'b1;
  assign o_f_addr = pc_q;
  assign instr = i_f_data;
  assign instr_opc = instr.opcode;
  assign instr_opr = instr.operand;
  assign instr_svc = instr.opcode == hex_pkg::OPR && instr.operand == hex_pkg::SVC;

  // Current operand (oreg) value.
  assign opr_d = oreg_q | {28'b0, instr.operand};

  // PC update
  always_comb begin
    pc_d = {pc_q + 18'b1};
    unique case (instr.opcode)
      hex_pkg::BR:
        pc_d = {pc_d + signed'(opr_d[hex_pkg::MEM_ADDR_WIDTH-1:0])};
      hex_pkg::BRZ:
        pc_d = (areg_q == '0) ? {pc_d + signed'(opr_d[hex_pkg::MEM_ADDR_WIDTH-1:0])} : pc_d;
      hex_pkg::BRN:
        pc_d = (signed'(areg_q) < 0) ? {pc_d + signed'(opr_d[hex_pkg::MEM_ADDR_WIDTH-1:0])} : pc_d;
      hex_pkg::OPR:
        pc_d = (instr.operand == hex_pkg::BRB) ? breg_q[hex_pkg::MEM_ADDR_WIDTH-1:0] : pc_d;
      default:;
    endcase
  end

  // oreg update
  always_comb begin
    unique case (instr.opcode)
      hex_pkg::PFIX: oreg_d = opr_d << 4;
      hex_pkg::NFIX: oreg_d = 32'hFFFFFF00 | (opr_d << 4);
      default:       oreg_d = 0;
    endcase
  end

  // areg update
  always_comb begin
    areg_d = areg_q;
    unique case (instr.opcode)
      hex_pkg::LDAM: areg_d = i_d_data;
      hex_pkg::LDAC: areg_d = opr_d;
      hex_pkg::LDAP: areg_d = {14'b0, pc_d + signed'(opr_d[hex_pkg::MEM_ADDR_WIDTH-1:0])};
      hex_pkg::LDAI: areg_d = i_d_data;
      hex_pkg::OPR:
        unique case(instr.operand)
          hex_pkg::ADD: areg_d = {areg_q + breg_q};
          hex_pkg::SUB: areg_d = {areg_q - breg_q};
          default:;
        endcase
      default:;
    endcase
  end

  // breg update
  always_comb begin
    breg_d = breg_q;
    unique case (instr.opcode)
      hex_pkg::LDBM,
      hex_pkg::LDBI: breg_d = i_d_data;
      hex_pkg::LDBC: breg_d = opr_d;
      default:;
    endcase
  end

  // Memory valid
  assign o_d_valid = instr.opcode inside {hex_pkg::LDAM, hex_pkg::LDBM, hex_pkg::STAM,
                                          hex_pkg::LDAI, hex_pkg::LDBI, hex_pkg::STAI};

  // Memory write enable
  assign o_d_we = instr.opcode inside {hex_pkg::STAM, hex_pkg::STAI};

  // Memory address generation
  always_comb begin
    o_d_addr = '0;
    unique case (instr.opcode)
      hex_pkg::LDAM,
      hex_pkg::LDBM,
      hex_pkg::STAM:
        o_d_addr = opr_d[hex_pkg::MEM_ADDR_WIDTH-3:0];
      hex_pkg::LDAI:
        o_d_addr = {areg_q[hex_pkg::MEM_ADDR_WIDTH-3:0]
                     + signed'(opr_d[hex_pkg::MEM_ADDR_WIDTH-3:0])};
      hex_pkg::LDBI,
      hex_pkg::STAI:
        o_d_addr = {breg_q[hex_pkg::MEM_ADDR_WIDTH-3:0]
                     + signed'(opr_d[hex_pkg::MEM_ADDR_WIDTH-3:0])};
      default:;
    endcase
  end

  // Data is always driven by the areg.
  assign o_d_data = areg_q;

  // Syscalls
  assign o_syscall_valid = instr_svc;
  assign o_syscall = hex_pkg::syscall_t'(areg_q);

endmodule
