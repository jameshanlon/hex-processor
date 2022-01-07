package hex_pkg;

  localparam MEM_ADDR_WIDTH = 21;
  localparam MEM_ADDR_MSB = MEM_ADDR_WIDTH-1;
  localparam MEM_ADDR_WORD_LSB = 2;
  localparam MEM_ADDR_BYTE_LSB = 0;
  localparam MEM_WIDTH = 32;
  localparam MEM_DEPTH = 1 << (MEM_ADDR_WIDTH - 2);
  localparam DATA_WIDTH = 32;
  localparam INSTR_OPC_WIDTH = 4;
  localparam INSTR_OPR_WIDTH = 4;
  localparam INSTR_WIDTH = INSTR_OPC_WIDTH + INSTR_OPR_WIDTH;
  localparam SYSCALL_OPC_WIDTH = 2;

  typedef logic [MEM_ADDR_WIDTH-1:0]  iaddr_t; // Byte/instruction address
  typedef logic [MEM_ADDR_WIDTH-1:2]  waddr_t; // Word address
  typedef logic [MEM_WIDTH-1:0]       data_t;
  typedef logic [INSTR_OPR_WIDTH-1:0] operand_t;

  typedef enum logic [INSTR_OPC_WIDTH-1:0] {
    LDAM = 'h0,
    LDBM = 'h1,
    STAM = 'h2,
    LDAC = 'h3,
    LDBC = 'h4,
    LDAP = 'h5,
    LDAI = 'h6,
    LDBI = 'h7,
    STAI = 'h8,
    BR   = 'h9,
    BRZ  = 'hA,
    BRN  = 'hB,
    OPR  = 'hD,
    PFIX = 'hE,
    NFIX = 'hF
  } opcode_t;

  typedef enum logic [INSTR_OPR_WIDTH-1:0] {
    BRB = 0,
    ADD = 1,
    SUB = 2,
    SVC = 3
  } opr_opcode_t;

  typedef enum logic [SYSCALL_OPC_WIDTH-1:0] {
    EXIT  = 0,
    WRITE = 1,
    READ  = 2
  } syscall_t;

  typedef struct packed {
    opcode_t  opcode;
    operand_t operand;
  } instr_t;

endpackage
