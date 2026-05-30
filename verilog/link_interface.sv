module link_interface (
    input  logic               i_clk,
    input  logic               i_rst,
    input  hex_pkg::core_id_t  i_core_id,
    // Processor side.
    input  logic               i_op_out,
    input  logic               i_op_in,
    input  hex_pkg::slot_t     i_slot,
    input  hex_pkg::data_t     i_areg,
    output logic               o_busy,
    output logic               o_done,
    output hex_pkg::data_t     o_in_word,
    // Route table config (reset-time).
    input  logic               i_cfg_we,
    input  hex_pkg::slot_t     i_cfg_slot,
    input  hex_pkg::core_id_t  i_cfg_dst_core,
    input  hex_pkg::slot_t     i_cfg_dst_slot,
    // DATA network.
    output logic                o_dnet_valid,
    output hex_pkg::core_id_t   o_dnet_dst,
    output hex_pkg::data_flit_t o_dnet_flit,
    input  logic                i_dnet_in_ready,
    input  logic                i_dnet_valid,
    input  hex_pkg::data_flit_t i_dnet_flit,
    output logic                o_dnet_out_ready,
    // ACK network.
    output logic               o_anet_valid,
    output hex_pkg::core_id_t  o_anet_dst,
    output hex_pkg::ack_flit_t o_anet_flit,
    input  logic               i_anet_in_ready,
    input  logic               i_anet_valid,
    input  hex_pkg::ack_flit_t i_anet_flit,
    output logic               o_anet_out_ready
  );

  // Route table: slot -> (dst_core, dst_slot).
  hex_pkg::core_id_t rt_core [hex_pkg::NUM_LINKS];
  hex_pkg::slot_t    rt_slot [hex_pkg::NUM_LINKS];
  always_ff @(posedge i_clk)
    if (i_cfg_we) begin
      rt_core[i_cfg_slot] <= i_cfg_dst_core;
      rt_slot[i_cfg_slot] <= i_cfg_dst_slot;
    end

  // Per-slot receive buffers.
  logic [hex_pkg::NUM_LINKS-1:0] rx_valid;
  hex_pkg::core_id_t             rx_src  [hex_pkg::NUM_LINKS];
  hex_pkg::data_t                rx_word [hex_pkg::NUM_LINKS];

  // Accept a delivered DATA flit into its (provably empty) slot buffer.
  // (i_dnet_flit.dst_core is not needed: flit is already routed to this core.)
  assign o_dnet_out_ready = i_dnet_valid && !rx_valid[i_dnet_flit.dst_slot];
  // ACKs are always accepted (this core is the waiting writer).
  // (i_anet_flit carries no payload; only i_anet_valid is inspected.)
  assign o_anet_out_ready = 1'b1;

  typedef enum logic [1:0] { IDLE, OUT_SEND, OUT_WAIT, IN_ACK } state_t;
  state_t st;

  // Combinational outputs.
  always_comb begin
    o_dnet_valid = 1'b0;
    o_dnet_dst   = '0;
    o_dnet_flit  = '0;
    o_anet_valid = 1'b0;
    o_anet_dst   = '0;
    o_anet_flit  = '0;
    o_done       = 1'b0;
    o_in_word    = rx_word[i_slot];
    o_busy       = (i_op_out || i_op_in);
    unique case (st)
      OUT_SEND: begin
        o_dnet_valid         = 1'b1;
        o_dnet_dst           = rt_core[i_slot];
        o_dnet_flit.dst_core = rt_core[i_slot];
        o_dnet_flit.dst_slot = rt_slot[i_slot];
        o_dnet_flit.src_core = i_core_id;
        o_dnet_flit.word     = i_areg;
      end
      OUT_WAIT: begin
        if (i_anet_valid) o_done = 1'b1; // ACK for this core
      end
      IN_ACK: begin
        o_anet_valid         = 1'b1;
        o_anet_dst           = rx_src[i_slot];
        o_anet_flit.dst_core = rx_src[i_slot];
        if (i_anet_in_ready) o_done = 1'b1;
      end
      default: ; // IDLE
    endcase
    if (o_done) o_busy = 1'b0;
  end

  // Sequential: FSM + receive buffers.
  always_ff @(posedge i_clk or posedge i_rst)
    if (i_rst) begin
      st <= IDLE;
      rx_valid <= '0;
    end else begin
      // Receive a delivered DATA flit.
      if (i_dnet_valid && o_dnet_out_ready) begin
        rx_valid[i_dnet_flit.dst_slot] <= 1'b1;
        rx_src  [i_dnet_flit.dst_slot] <= i_dnet_flit.src_core;
        rx_word [i_dnet_flit.dst_slot] <= i_dnet_flit.word;
      end
      // FSM.
      unique case (st)
        IDLE: begin
          if (i_op_out)                         st <= OUT_SEND;
          else if (i_op_in && rx_valid[i_slot]) st <= IN_ACK;
        end
        OUT_SEND: if (i_dnet_in_ready) st <= OUT_WAIT;
        OUT_WAIT: if (i_anet_valid)    st <= IDLE;
        IN_ACK:   if (i_anet_in_ready) begin
                    rx_valid[i_slot] <= 1'b0;
                    st <= IDLE;
                  end
      endcase
    end

endmodule
