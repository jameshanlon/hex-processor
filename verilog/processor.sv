module processor
  (
    input logic               i_rst,
    input logic               i_clk,
    // Fetch memory port
    output logic              o_f_valid,
    output hex_pkg::addr_t    o_f_addr,
    input  hex_pkg::instr_t   i_f_data,
    // Data memory port
    output logic              o_d_valid,
    output logic              o_d_we,
    output hex_pkg::addr_t    o_d_addr,
    output hex_pkg::data_t    o_d_data,
    input  hex_pkg::data_t    i_d_data,
    // Syscall interface
    output logic              o_syscall_valid,
    output hex_pkg::syscall_t o_syscall
  );

  // State
  hex_pkg::addr_t  pc_q;
  hex_pkg::data_t  areg_q;
  hex_pkg::data_t  breg_q;
  hex_pkg::data_t  oreg_q;

  // Nets
  hex_pkg::instr_t instr;
  logic            instr_svc;
  hex_pkg::addr_t  pc_d;
  hex_pkg::data_t  areg_d;
  hex_pkg::data_t  breg_d;
  hex_pkg::data_t  oreg_d;
  hex_pkg::data_t  opr_d;
  hex_pkg::addr_t  oreg_addr;
  hex_pkg::addr_t  areg_addr;
  hex_pkg::addr_t  breg_addr;
  hex_pkg::addr_t  opr_addr;
  hex_pkg::addr_t  opr_word_addr;

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

  // Truncated versions of signals for address arithmetic.
  assign oreg_addr = oreg_q[hex_pkg::MEM_ADDR_MSB:hex_pkg::MEM_ADDR_LSB];
  assign areg_addr = areg_q[hex_pkg::MEM_ADDR_MSB:hex_pkg::MEM_ADDR_LSB];
  assign breg_addr = breg_q[hex_pkg::MEM_ADDR_MSB:hex_pkg::MEM_ADDR_LSB];
  assign opr_addr = opr_d[hex_pkg::MEM_ADDR_MSB:hex_pkg::MEM_ADDR_LSB];
  assign opr_word_addr = {opr_d[hex_pkg::MEM_ADDR_MSB-2:hex_pkg::MEM_ADDR_LSB], 2'b0};

  // Instruction fetch.
  assign o_f_valid = 1'b1;
  assign o_f_addr = pc_q;
  assign instr = i_f_data;
  assign instr_svc = instr.opc == hex_pkg::OPR && instr.opr == hex_pkg::SVC;

  // Current operand (oreg) value.
  assign opr_d = oreg_q | {28'b0, instr.opr};

  // PC update
  always_comb begin
    pc_d = {pc_q + 18'b1};
    unique case (instr.opc)
      hex_pkg::BR:  pc_d = {pc_d + oreg_addr};
      hex_pkg::BRZ: pc_d = (areg_q == '0) ? {pc_d + oreg_addr} : pc_d;
      hex_pkg::BRN: pc_d = (signed'(areg_q) < 0) ? {pc_d + oreg_addr} : pc_d;
      hex_pkg::OPR: pc_d = (instr.opr == hex_pkg::BRB) ? breg_addr : pc_d;
      default:;
    endcase
  end

  // oreg update
  always_comb begin
    unique case (instr.opc)
      hex_pkg::PFIX: oreg_d = oreg_q << 4;
      hex_pkg::NFIX: oreg_d = 32'hFFFFFF00 | (oreg_q << 4);
      default:                oreg_d = 0;
    endcase
  end

  // areg update
  always_comb begin
    areg_d = areg_q;
    unique case (instr.opc)
      hex_pkg::LDAM: areg_d = o_d_data;
      hex_pkg::LDAC: areg_d = opr_d;
      hex_pkg::LDAP: areg_d = {14'b0, pc_d + opr_addr};
      hex_pkg::LDAI: areg_d = o_d_data;
      hex_pkg::OPR:
        unique case(instr.opr)
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
    unique case (instr.opc)
      hex_pkg::LDBM: breg_d = o_d_data;
      hex_pkg::LDBC: breg_d = opr_d;
      hex_pkg::LDBI: breg_d = o_d_data;
      default:;
    endcase
  end

  // Memory valid
  assign o_d_valid = instr.opc inside {hex_pkg::LDAM, hex_pkg::LDBM, hex_pkg::STAM,
                                       hex_pkg::LDAI, hex_pkg::LDBI, hex_pkg::STAI} /*||
                                      instr_svc*/;

  // Memory write enable
  assign o_d_we = instr.opc inside {hex_pkg::STAM, hex_pkg::STAI};

  // Memory address generation
  always_comb begin
    o_d_addr = '0;
    unique case (instr.opc)
      hex_pkg::LDAM,
      hex_pkg::LDBM,
      hex_pkg::STAM: o_d_addr = opr_addr;
      hex_pkg::LDAI: o_d_addr = {areg_addr + opr_word_addr};
      hex_pkg::LDBI,
      hex_pkg::STAI: o_d_addr = {breg_addr + opr_word_addr};
      //hex_pkg::OPR:  o_d_addr = (instr.opr == hex_pkg::SVC) ? 18'd4;
      default:;
    endcase
  end

  // Data is always driven by the areg.
  assign o_d_data = areg_q;

  // Syscalls
  assign o_syscall_valid = instr_svc;
  assign o_syscall = hex_pkg::syscall_t'(areg_q);

endmodule
