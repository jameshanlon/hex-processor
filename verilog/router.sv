module router #(
    parameter int FLIT_W = 32
  ) (
    input  logic                          i_clk,
    input  logic                          i_rst,
    // Injection ports (from cores).
    input  logic [hex_pkg::NUM_CORES-1:0] i_in_valid,
    input  hex_pkg::core_id_t             i_in_dst   [hex_pkg::NUM_CORES],
    input  logic [FLIT_W-1:0]             i_in_flit  [hex_pkg::NUM_CORES],
    output logic [hex_pkg::NUM_CORES-1:0] o_in_ready,
    // Delivery ports (to cores).
    output logic [hex_pkg::NUM_CORES-1:0] o_out_valid,
    output logic [FLIT_W-1:0]             o_out_flit [hex_pkg::NUM_CORES],
    input  logic [hex_pkg::NUM_CORES-1:0] i_out_ready
  );

  localparam int N = hex_pkg::NUM_CORES;

  // 1-deep input buffers.
  logic [N-1:0]         ibuf_valid;
  hex_pkg::core_id_t    ibuf_dst  [N];
  logic [FLIT_W-1:0]    ibuf_flit [N];

  // Registered outputs.
  logic [N-1:0]         obuf_valid;
  logic [FLIT_W-1:0]    obuf_flit [N];

  // Round-robin priority pointer per output.
  hex_pkg::core_id_t    rr [N];

  // Accept a new injection only when the input buffer is empty.
  always_comb
    for (int i = 0; i < N; i++)
      o_in_ready[i] = !ibuf_valid[i];

  // Per-output round-robin grant among input buffers addressed to it.
  logic [N-1:0]      grant_vld;
  hex_pkg::core_id_t grant_idx [N];
  always_comb begin
    for (int j = 0; j < N; j++) begin
      grant_vld[j] = 1'b0;
      grant_idx[j] = '0;
      for (int k = 0; k < N; k++) begin
        automatic hex_pkg::core_id_t i = hex_pkg::core_id_t'((int'(rr[j]) + k) % N);
        if (!grant_vld[j] && ibuf_valid[i] && (ibuf_dst[i] == j[hex_pkg::CID_W-1:0])) begin
          grant_vld[j] = 1'b1;
          grant_idx[j] = i;
        end
      end
    end
  end

  assign o_out_valid = obuf_valid;
  always_comb
    for (int j = 0; j < N; j++)
      o_out_flit[j] = obuf_flit[j];

  always_ff @(posedge i_clk or posedge i_rst)
    if (i_rst) begin
      ibuf_valid <= '0;
      obuf_valid <= '0;
      for (int i = 0; i < N; i++) rr[i] <= '0;
    end else begin
      // Load input buffers from injectors.
      for (int i = 0; i < N; i++)
        if (!ibuf_valid[i] && i_in_valid[i]) begin
          ibuf_valid[i] <= 1'b1;
          ibuf_dst[i]   <= i_in_dst[i];
          ibuf_flit[i]  <= i_in_flit[i];
        end
      // Drain and (re)fill each output. A flit is granted to output j only when
      // that output will be free this cycle, so a stalled output (i_out_ready
      // low) holds its flit and back-pressures the input buffer (no flit loss).
      for (int j = 0; j < N; j++) begin
        if (obuf_valid[j] && i_out_ready[j])
          obuf_valid[j] <= 1'b0;
        if ((!obuf_valid[j] || i_out_ready[j]) && grant_vld[j]) begin
          obuf_valid[j]            <= 1'b1;
          obuf_flit[j]             <= ibuf_flit[grant_idx[j]];
          ibuf_valid[grant_idx[j]] <= 1'b0;
          rr[j]                    <= hex_pkg::core_id_t'((int'(grant_idx[j]) + 1) % N);
        end
      end
    end

endmodule
