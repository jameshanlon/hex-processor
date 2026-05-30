// A fixed network of NUM_CORES cores connected by a DATA router and an ACK
// router. The wiring is static; the per-core route tables (slot -> address)
// are programmed at reset via the config port, so any topology that fits is
// realised by configuration rather than re-elaboration.
//
// Core k injects into router input port k and is delivered from router output
// port k. A flit's dst_core selects the destination output port.
module network_top
  (
    input  logic               i_clk,
    input  logic               i_rst,
    // Per-core syscall interface.
    output logic [hex_pkg::NUM_CORES-1:0] o_syscall_valid,
    output hex_pkg::syscall_t             o_syscall [hex_pkg::NUM_CORES],
    // Route-table config: write (dst_core,dst_slot) to core i_cfg_core slot
    // i_cfg_slot when i_cfg_we is high.
    input  logic               i_cfg_we,
    input  hex_pkg::core_id_t  i_cfg_core,
    input  hex_pkg::slot_t     i_cfg_slot,
    input  hex_pkg::core_id_t  i_cfg_dst_core,
    input  hex_pkg::slot_t     i_cfg_dst_slot
  );

  localparam int N = hex_pkg::NUM_CORES;

  // DATA network wiring (core k <-> router port k).
  logic [N-1:0]        dnet_in_valid;
  hex_pkg::core_id_t   dnet_in_dst   [N];
  hex_pkg::data_flit_t dnet_in_flit  [N];
  logic [N-1:0]        dnet_in_ready;
  logic [N-1:0]        dnet_out_valid;
  hex_pkg::data_flit_t dnet_out_flit [N];
  logic [N-1:0]        dnet_out_ready;

  // ACK network wiring.
  logic [N-1:0]        anet_in_valid;
  hex_pkg::core_id_t   anet_in_dst   [N];
  hex_pkg::ack_flit_t  anet_in_flit  [N];
  logic [N-1:0]        anet_in_ready;
  logic [N-1:0]        anet_out_valid;
  hex_pkg::ack_flit_t  anet_out_flit [N];
  logic [N-1:0]        anet_out_ready;

  // Per-core config write enable (decoded from i_cfg_core).
  logic [N-1:0] cfg_we;
  always_comb
    for (int k = 0; k < N; k++)
      cfg_we[k] = i_cfg_we && (i_cfg_core == k[hex_pkg::CID_W-1:0]);

  genvar k;
  generate
    for (k = 0; k < N; k++) begin : g_core
      core u_core (
        .i_clk            (i_clk),
        .i_rst            (i_rst),
        .i_core_id        (k[hex_pkg::CID_W-1:0]),
        .o_syscall_valid  (o_syscall_valid[k]),
        .o_syscall        (o_syscall[k]),
        .i_cfg_we         (cfg_we[k]),
        .i_cfg_slot       (i_cfg_slot),
        .i_cfg_dst_core   (i_cfg_dst_core),
        .i_cfg_dst_slot   (i_cfg_dst_slot),
        .o_dnet_valid     (dnet_in_valid[k]),
        .o_dnet_dst       (dnet_in_dst[k]),
        .o_dnet_flit      (dnet_in_flit[k]),
        .i_dnet_in_ready  (dnet_in_ready[k]),
        .i_dnet_valid     (dnet_out_valid[k]),
        .i_dnet_flit      (dnet_out_flit[k]),
        .o_dnet_out_ready (dnet_out_ready[k]),
        .o_anet_valid     (anet_in_valid[k]),
        .o_anet_dst       (anet_in_dst[k]),
        .o_anet_flit      (anet_in_flit[k]),
        .i_anet_in_ready  (anet_in_ready[k]),
        .i_anet_valid     (anet_out_valid[k]),
        .i_anet_flit      (anet_out_flit[k]),
        .o_anet_out_ready (anet_out_ready[k])
      );
    end
  endgenerate

  router #(.FLIT_W($bits(hex_pkg::data_flit_t))) u_dnet (
    .i_clk       (i_clk),
    .i_rst       (i_rst),
    .i_in_valid  (dnet_in_valid),
    .i_in_dst    (dnet_in_dst),
    .i_in_flit   (dnet_in_flit),
    .o_in_ready  (dnet_in_ready),
    .o_out_valid (dnet_out_valid),
    .o_out_flit  (dnet_out_flit),
    .i_out_ready (dnet_out_ready)
  );

  router #(.FLIT_W($bits(hex_pkg::ack_flit_t))) u_anet (
    .i_clk       (i_clk),
    .i_rst       (i_rst),
    .i_in_valid  (anet_in_valid),
    .i_in_dst    (anet_in_dst),
    .i_in_flit   (anet_in_flit),
    .o_in_ready  (anet_in_ready),
    .o_out_valid (anet_out_valid),
    .o_out_flit  (anet_out_flit),
    .i_out_ready (anet_out_ready)
  );

endmodule
