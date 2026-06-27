// A single processor core: processor + private memory + per-core link
// interface, exposing the syscall interface, the route-table config port, and
// the DATA/ACK network ports for connection to the router(s).
module core
  (
    input  logic               i_clk,
    input  logic               i_rst,
    input  hex_pkg::core_id_t  i_core_id,
    // Syscall interface
    output logic               o_syscall_valid,
    output hex_pkg::syscall_t  o_syscall,
    // Route-table config (reset-time).
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

  // Shared single memory port.
  logic             mem_valid;
  logic             mem_we;
  hex_pkg::waddr_t  mem_addr;
  hex_pkg::data_t   mem_wdata;
  hex_pkg::data_t   mem_rdata;

  // Processor <-> link interface.
  logic           op_out;
  logic           op_in;
  hex_pkg::slot_t chan_slot;
  hex_pkg::data_t chan_areg;
  logic           liu_busy;
  logic           liu_done;
  hex_pkg::data_t liu_in_word;

  processor u_processor (
    .i_rst           (i_rst),
    .i_clk           (i_clk),
    .o_mem_valid     (mem_valid),
    .o_mem_we        (mem_we),
    .o_mem_addr      (mem_addr),
    .o_mem_wdata     (mem_wdata),
    .i_mem_rdata     (mem_rdata),
    .o_syscall_valid (o_syscall_valid),
    .o_syscall       (o_syscall),
    .o_op_out        (op_out),
    .o_op_in         (op_in),
    .o_chan_slot     (chan_slot),
    .o_chan_areg     (chan_areg),
    .i_liu_busy      (liu_busy),
    .i_liu_in_word   (liu_in_word)
  );

  memory u_memory (
    .i_clk   (i_clk),
    .i_valid (mem_valid),
    .i_we    (mem_we),
    .i_addr  (mem_addr),
    .i_wdata (mem_wdata),
    .o_rdata (mem_rdata)
  );

  link_interface u_liu (
    .i_clk            (i_clk),
    .i_rst            (i_rst),
    .i_core_id        (i_core_id),
    .i_op_out         (op_out),
    .i_op_in          (op_in),
    .i_slot           (chan_slot),
    .i_areg           (chan_areg),
    .o_busy           (liu_busy),
    .o_done           (liu_done),
    .o_in_word        (liu_in_word),
    .i_cfg_we         (i_cfg_we),
    .i_cfg_slot       (i_cfg_slot),
    .i_cfg_dst_core   (i_cfg_dst_core),
    .i_cfg_dst_slot   (i_cfg_dst_slot),
    .o_dnet_valid     (o_dnet_valid),
    .o_dnet_dst       (o_dnet_dst),
    .o_dnet_flit      (o_dnet_flit),
    .i_dnet_in_ready  (i_dnet_in_ready),
    .i_dnet_valid     (i_dnet_valid),
    .i_dnet_flit      (i_dnet_flit),
    .o_dnet_out_ready (o_dnet_out_ready),
    .o_anet_valid     (o_anet_valid),
    .o_anet_dst       (o_anet_dst),
    .o_anet_flit      (o_anet_flit),
    .i_anet_in_ready  (i_anet_in_ready),
    .i_anet_valid     (i_anet_valid),
    .i_anet_flit      (i_anet_flit),
    .o_anet_out_ready (o_anet_out_ready)
  );

endmodule
