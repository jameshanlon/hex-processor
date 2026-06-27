module processor
  (
    input logic               i_rst,
    input logic               i_clk,
    // Shared single memory port (one-cycle synchronous read latency).
    output logic              o_mem_valid,
    output logic              o_mem_we,
    output hex_pkg::waddr_t   o_mem_addr,
    output hex_pkg::data_t    o_mem_wdata,
    input  hex_pkg::data_t    i_mem_rdata,
    // Syscall interface
    output logic              o_syscall_valid,
    output hex_pkg::syscall_t o_syscall,
    // Channel link interface (to the per-core link_interface unit).
    output logic              o_op_out,
    output logic              o_op_in,
    output hex_pkg::slot_t    o_chan_slot,
    output hex_pkg::data_t    o_chan_areg,
    input  logic              i_liu_busy,
    input  hex_pkg::data_t    i_liu_in_word
  );

  // Memory access has a one-cycle read latency, so each instruction runs as a
  // short sequence: present the fetch address (S_FETCH), then decode/execute the
  // returned word (S_EXEC). Loads need a third step (S_LOAD) to consume the data
  // word the cycle after its address was presented. The single memory port is
  // shared: fetch uses it in S_FETCH, loads/stores in S_EXEC; they never collide.
  typedef enum logic [1:0] { S_FETCH, S_EXEC, S_LOAD } state_t;

  // State
  state_t            state_q;
  hex_pkg::iaddr_t   pc_q /* verilator public */;
  hex_pkg::data_t    areg_q;
  hex_pkg::data_t    breg_q;
  hex_pkg::data_t    oreg_q;
  hex_pkg::instr_t   instr_q;  // Instruction held across a load's S_LOAD cycle.

  // Nets
  state_t            state_d;
  hex_pkg::instr_t   instr /* verilator public */;
  hex_pkg::instr_t   fetched_instr;
  logic              is_load;
  logic              is_store;
  logic              instr_svc;
  logic              instr_in;
  logic              instr_out;
  logic              exec_phase; // Asserted while the instruction is executing.
  logic              stall;
  logic              commit;
  hex_pkg::iaddr_t   pc_d;
  hex_pkg::data_t    areg_d;
  hex_pkg::data_t    breg_d;
  hex_pkg::data_t    oreg_d;
  hex_pkg::data_t    opr_d;
  hex_pkg::waddr_t   data_addr;
  logic [4:0]        fetch_byte_off; // Byte index of the fetched instruction.

  always_ff @(posedge i_clk or posedge i_rst)
    if (i_rst) begin
      state_q <= S_FETCH;
      pc_q    <= '0;
      areg_q  <= '0;
      breg_q  <= '0;
      oreg_q  <= '0;
      instr_q <= '0;
    end else begin
      state_q <= state_d;
      if (state_q == S_EXEC)
        instr_q <= instr;
      if (commit) begin
        pc_q   <= pc_d;
        areg_q <= areg_d;
        breg_q <= breg_d;
        oreg_q <= oreg_d;
      end
    end

  // Next-state and commit. An instruction completes (commits its results and
  // advances the PC) in S_EXEC for non-loads, or in S_LOAD for loads. Channel
  // ops freeze the pipeline in S_EXEC until the link interface completes.
  always_comb begin
    state_d = state_q;
    commit  = 1'b0;
    unique case (state_q)
      S_FETCH: state_d = S_EXEC;
      S_EXEC:
        if (!stall) begin
          if (is_load) begin
            state_d = S_LOAD;
          end else begin
            state_d = S_FETCH;
            commit  = 1'b1;
          end
        end
      S_LOAD: begin
        state_d = S_FETCH;
        commit  = 1'b1;
      end
      default: state_d = S_FETCH;
    endcase
  end

  // The fetched instruction is the byte selected from the returned word; in
  // S_LOAD the read port holds the load's data, so use the latched instruction.
  assign fetch_byte_off = {3'b000, pc_q[1:0]} << 3;
  assign fetched_instr  = hex_pkg::instr_t'(i_mem_rdata[fetch_byte_off +: 8]);
  assign instr          = (state_q == S_LOAD) ? instr_q : fetched_instr;
  assign exec_phase     = (state_q == S_EXEC);
  assign is_load        = instr.opcode inside {hex_pkg::LDAM, hex_pkg::LDBM,
                                               hex_pkg::LDAI, hex_pkg::LDBI};
  assign is_store       = instr.opcode inside {hex_pkg::STAM, hex_pkg::STAI};
  assign instr_svc      = instr.opcode == hex_pkg::OPR && instr.operand == hex_pkg::SVC;
  assign instr_in       = instr.opcode == hex_pkg::OPR && instr.operand == hex_pkg::IN;
  assign instr_out      = instr.opcode == hex_pkg::OPR && instr.operand == hex_pkg::OUT;

  // Channel ops hand off to the link interface; the channel index is in breg
  // and the word to send is in areg. The processor stalls (freezes all state)
  // until the link interface completes the rendezvous. Strobes are qualified by
  // S_EXEC so they assert exactly once per instruction.
  assign o_op_out    = instr_out && exec_phase;
  assign o_op_in     = instr_in  && exec_phase;
  assign o_chan_slot = breg_q[hex_pkg::SLOT_W-1:0];
  assign o_chan_areg = areg_q;
  assign stall       = (instr_in || instr_out) && i_liu_busy && exec_phase;

  // Current operand (oreg) value.
  assign opr_d = oreg_q | {28'b0, instr.operand};

  // PC update
  always_comb begin
    pc_d = {pc_q + 20'b1};
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

  // areg update. Loads take their value from the read port, consumed in S_LOAD.
  always_comb begin
    areg_d = areg_q;
    unique case (instr.opcode)
      hex_pkg::LDAM: areg_d = i_mem_rdata;
      hex_pkg::LDAC: areg_d = opr_d;
      hex_pkg::LDAP: areg_d = {11'b0, pc_d + signed'(opr_d[hex_pkg::MEM_ADDR_WIDTH-1:0])};
      hex_pkg::LDAI: areg_d = i_mem_rdata;
      hex_pkg::OPR:
        unique case(instr.operand)
          hex_pkg::ADD: areg_d = {areg_q + breg_q};
          hex_pkg::SUB: areg_d = {areg_q - breg_q};
          hex_pkg::IN:  areg_d = i_liu_in_word;
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
      hex_pkg::LDBI: breg_d = i_mem_rdata;
      hex_pkg::LDBC: breg_d = opr_d;
      default:;
    endcase
  end

  // Data-access word address generation (loads and stores).
  always_comb begin
    data_addr = '0;
    unique case (instr.opcode)
      hex_pkg::LDAM,
      hex_pkg::LDBM,
      hex_pkg::STAM:
        data_addr = opr_d[hex_pkg::MEM_ADDR_WIDTH-3:0];
      hex_pkg::LDAI:
        data_addr = {areg_q[hex_pkg::MEM_ADDR_WIDTH-3:0]
                      + signed'(opr_d[hex_pkg::MEM_ADDR_WIDTH-3:0])};
      hex_pkg::LDBI,
      hex_pkg::STAI:
        data_addr = {breg_q[hex_pkg::MEM_ADDR_WIDTH-3:0]
                      + signed'(opr_d[hex_pkg::MEM_ADDR_WIDTH-3:0])};
      default:;
    endcase
  end

  // Shared memory port: fetch in S_FETCH, data access in S_EXEC. The store write
  // commits on the clock edge ending S_EXEC; the load read result appears the
  // following cycle (S_LOAD).
  always_comb begin
    o_mem_valid = 1'b0;
    o_mem_we    = 1'b0;
    o_mem_addr  = pc_q[hex_pkg::MEM_ADDR_MSB:hex_pkg::MEM_ADDR_WORD_LSB];
    o_mem_wdata = areg_q;
    unique case (state_q)
      S_FETCH: o_mem_valid = 1'b1; // Address defaults to the fetch (PC) address.
      S_EXEC:
        if (is_store) begin
          o_mem_valid = 1'b1;
          o_mem_we    = 1'b1;
          o_mem_addr  = data_addr;
          o_mem_wdata = areg_q;
        end else if (is_load) begin
          o_mem_valid = 1'b1;
          o_mem_addr  = data_addr;
        end
      default:; // S_LOAD: port idle, consuming the registered read data.
    endcase
  end

  // Syscalls (qualified by exec_phase so the strobe pulses once).
  assign o_syscall_valid = instr_svc && exec_phase;
  assign o_syscall = hex_pkg::syscall_t'(areg_q);

endmodule
