module processor (
	i_rst,
	i_clk,
	o_f_valid,
	o_f_addr,
	i_f_data,
	o_d_valid,
	o_d_we,
	o_d_addr,
	o_d_data,
	i_d_data,
	o_syscall_valid,
	o_syscall
);
	input wire i_rst;
	input wire i_clk;
	output wire o_f_valid;
	localparam hex_pkg_MEM_ADDR_WIDTH = 21;
	output wire [20:0] o_f_addr;
	localparam hex_pkg_INSTR_OPC_WIDTH = 4;
	localparam hex_pkg_INSTR_OPR_WIDTH = 4;
	input wire [7:0] i_f_data;
	output wire o_d_valid;
	output wire o_d_we;
	output reg [20:2] o_d_addr;
	localparam hex_pkg_MEM_WIDTH = 32;
	output wire [31:0] o_d_data;
	input wire [31:0] i_d_data;
	output wire o_syscall_valid;
	localparam hex_pkg_SYSCALL_OPC_WIDTH = 2;
	output wire [1:0] o_syscall;
	reg [20:0] pc_q;
	reg [31:0] areg_q;
	reg [31:0] breg_q;
	reg [31:0] oreg_q;
	wire [7:0] instr;
	wire instr_svc;
	reg [20:0] pc_d;
	reg [31:0] areg_d;
	reg [31:0] breg_d;
	reg [31:0] oreg_d;
	wire [31:0] opr_d;
	wire [20:0] opr_iaddr;
	wire [3:0] instr_opc;
	wire [3:0] instr_opr;
	always @(posedge i_clk or posedge i_rst)
		if (i_rst) begin
			pc_q <= 1'sb0;
			areg_q <= 1'sb0;
			breg_q <= 1'sb0;
			oreg_q <= 1'sb0;
		end
		else begin
			pc_q <= pc_d;
			areg_q <= areg_d;
			breg_q <= breg_d;
			oreg_q <= oreg_d;
		end
	assign o_f_valid = 1'b1;
	assign o_f_addr = pc_q;
	assign instr = i_f_data;
	assign instr_opc = instr[7-:4];
	assign instr_opr = instr[3-:hex_pkg_INSTR_OPR_WIDTH];
	function automatic [3:0] sv2v_cast_024B2;
		input reg [3:0] inp;
		sv2v_cast_024B2 = inp;
	endfunction
	function automatic [3:0] sv2v_cast_4E38D;
		input reg [3:0] inp;
		sv2v_cast_4E38D = inp;
	endfunction
	assign instr_svc = (instr[7-:4] == sv2v_cast_024B2('hd)) && (instr[3-:hex_pkg_INSTR_OPR_WIDTH] == sv2v_cast_4E38D(3));
	assign opr_d = oreg_q | {28'b0000000000000000000000000000, instr[3-:hex_pkg_INSTR_OPR_WIDTH]};
	always @(*) begin
		pc_d = {pc_q + 20'b00000000000000000001};
		case (instr[7-:4])
			sv2v_cast_024B2('h9): pc_d = {pc_d + $signed(opr_d[20:0])};
			sv2v_cast_024B2('ha): pc_d = (areg_q == {32 {1'sb0}} ? {pc_d + $signed(opr_d[20:0])} : pc_d);
			sv2v_cast_024B2('hb): pc_d = ($signed(areg_q) < 0 ? {pc_d + $signed(opr_d[20:0])} : pc_d);
			sv2v_cast_024B2('hd): pc_d = (instr[3-:hex_pkg_INSTR_OPR_WIDTH] == sv2v_cast_4E38D(0) ? breg_q[20:0] : pc_d);
			default:
				;
		endcase
	end
	always @(*)
		case (instr[7-:4])
			sv2v_cast_024B2('he): oreg_d = opr_d << 4;
			sv2v_cast_024B2('hf): oreg_d = 32'hffffff00 | (opr_d << 4);
			default: oreg_d = 0;
		endcase
	always @(*) begin
		areg_d = areg_q;
		case (instr[7-:4])
			sv2v_cast_024B2('h0): areg_d = i_d_data;
			sv2v_cast_024B2('h3): areg_d = opr_d;
			sv2v_cast_024B2('h5): areg_d = {11'b00000000000, pc_d + $signed(opr_d[20:0])};
			sv2v_cast_024B2('h6): areg_d = i_d_data;
			sv2v_cast_024B2('hd):
				case (instr[3-:hex_pkg_INSTR_OPR_WIDTH])
					sv2v_cast_4E38D(1): areg_d = {areg_q + breg_q};
					sv2v_cast_4E38D(2): areg_d = {areg_q - breg_q};
					default:
						;
				endcase
			default:
				;
		endcase
	end
	always @(*) begin
		breg_d = breg_q;
		case (instr[7-:4])
			sv2v_cast_024B2('h1), sv2v_cast_024B2('h7): breg_d = i_d_data;
			sv2v_cast_024B2('h4): breg_d = opr_d;
			default:
				;
		endcase
	end
	assign o_d_valid = |{((sv2v_cast_024B2('h0) ^ (instr[7-:4] ^ instr[7-:4])) === (instr[7-:4] ^ (sv2v_cast_024B2('h0) ^ sv2v_cast_024B2('h0)))) & ((((instr[7-:4] ^ instr[7-:4]) ^ (sv2v_cast_024B2('h0) ^ sv2v_cast_024B2('h0))) === (sv2v_cast_024B2('h0) ^ sv2v_cast_024B2('h0))) | 1'bx), ((sv2v_cast_024B2('h1) ^ (instr[7-:4] ^ instr[7-:4])) === (instr[7-:4] ^ (sv2v_cast_024B2('h1) ^ sv2v_cast_024B2('h1)))) & ((((instr[7-:4] ^ instr[7-:4]) ^ (sv2v_cast_024B2('h1) ^ sv2v_cast_024B2('h1))) === (sv2v_cast_024B2('h1) ^ sv2v_cast_024B2('h1))) | 1'bx), ((sv2v_cast_024B2('h2) ^ (instr[7-:4] ^ instr[7-:4])) === (instr[7-:4] ^ (sv2v_cast_024B2('h2) ^ sv2v_cast_024B2('h2)))) & ((((instr[7-:4] ^ instr[7-:4]) ^ (sv2v_cast_024B2('h2) ^ sv2v_cast_024B2('h2))) === (sv2v_cast_024B2('h2) ^ sv2v_cast_024B2('h2))) | 1'bx), ((sv2v_cast_024B2('h6) ^ (instr[7-:4] ^ instr[7-:4])) === (instr[7-:4] ^ (sv2v_cast_024B2('h6) ^ sv2v_cast_024B2('h6)))) & ((((instr[7-:4] ^ instr[7-:4]) ^ (sv2v_cast_024B2('h6) ^ sv2v_cast_024B2('h6))) === (sv2v_cast_024B2('h6) ^ sv2v_cast_024B2('h6))) | 1'bx), ((sv2v_cast_024B2('h7) ^ (instr[7-:4] ^ instr[7-:4])) === (instr[7-:4] ^ (sv2v_cast_024B2('h7) ^ sv2v_cast_024B2('h7)))) & ((((instr[7-:4] ^ instr[7-:4]) ^ (sv2v_cast_024B2('h7) ^ sv2v_cast_024B2('h7))) === (sv2v_cast_024B2('h7) ^ sv2v_cast_024B2('h7))) | 1'bx), ((sv2v_cast_024B2('h8) ^ (instr[7-:4] ^ instr[7-:4])) === (instr[7-:4] ^ (sv2v_cast_024B2('h8) ^ sv2v_cast_024B2('h8)))) & ((((instr[7-:4] ^ instr[7-:4]) ^ (sv2v_cast_024B2('h8) ^ sv2v_cast_024B2('h8))) === (sv2v_cast_024B2('h8) ^ sv2v_cast_024B2('h8))) | 1'bx)};
	assign o_d_we = |{((sv2v_cast_024B2('h2) ^ (instr[7-:4] ^ instr[7-:4])) === (instr[7-:4] ^ (sv2v_cast_024B2('h2) ^ sv2v_cast_024B2('h2)))) & ((((instr[7-:4] ^ instr[7-:4]) ^ (sv2v_cast_024B2('h2) ^ sv2v_cast_024B2('h2))) === (sv2v_cast_024B2('h2) ^ sv2v_cast_024B2('h2))) | 1'bx), ((sv2v_cast_024B2('h8) ^ (instr[7-:4] ^ instr[7-:4])) === (instr[7-:4] ^ (sv2v_cast_024B2('h8) ^ sv2v_cast_024B2('h8)))) & ((((instr[7-:4] ^ instr[7-:4]) ^ (sv2v_cast_024B2('h8) ^ sv2v_cast_024B2('h8))) === (sv2v_cast_024B2('h8) ^ sv2v_cast_024B2('h8))) | 1'bx)};
	always @(*) begin
		o_d_addr = 1'sb0;
		case (instr[7-:4])
			sv2v_cast_024B2('h0), sv2v_cast_024B2('h1), sv2v_cast_024B2('h2): o_d_addr = opr_d[18:0];
			sv2v_cast_024B2('h6): o_d_addr = {areg_q[18:0] + $signed(opr_d[18:0])};
			sv2v_cast_024B2('h7), sv2v_cast_024B2('h8): o_d_addr = {breg_q[18:0] + $signed(opr_d[18:0])};
			default:
				;
		endcase
	end
	assign o_d_data = areg_q;
	assign o_syscall_valid = instr_svc;
	function automatic [1:0] sv2v_cast_6BA3B;
		input reg [1:0] inp;
		sv2v_cast_6BA3B = inp;
	endfunction
	assign o_syscall = sv2v_cast_6BA3B(areg_q);
endmodule
