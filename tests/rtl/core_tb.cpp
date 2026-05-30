#include <verilated.h>
#include "Vcore.h"
#include "Vcore_core.h"
#include "Vcore_memory.h"
#include "Vcore_processor.h"
#include <cassert>
#include <cstdint>
#include <cstdio>

// data_flit_t layout (38 bits, QData): [37:36]=dst_core [35:34]=dst_slot
// [33:32]=src_core [31:0]=word.
static uint32_t flit_word(uint64_t f) { return (uint32_t)(f & 0xFFFFFFFFu); }
static unsigned flit_src(uint64_t f) { return (f >> 32) & 0x3; }
static unsigned flit_dslot(uint64_t f) { return (f >> 34) & 0x3; }
static unsigned flit_dcore(uint64_t f) { return (f >> 36) & 0x3; }

static void tick(Vcore *d) { d->i_clk = 0; d->eval(); d->i_clk = 1; d->eval(); }

int main() {
  Vcore *d = new Vcore;

  // Program (byte addresses):
  //   0: PFIX 4 (0xE4)  1: LDAC 1 (0x31)  -> areg = 0x41 = 65 ('A')
  //   2: LDBC 0 (0x40)                     -> breg = 0 (channel slot 0)
  //   3: OPR OUT (0xD5)                    -> send areg on channel 0
  //   4: NFIX F (0xFF)  5: BR F (0x9F)     -> loop in place
  d->i_rst = 1; d->i_clk = 0; d->eval();
  d->core->u_memory->memory_q[0] = 0xD54031E4u;
  d->core->u_memory->memory_q[1] = 0x00009FFFu;

  d->i_core_id = 2;
  // Router is always ready to accept injected flits; nothing delivered yet.
  d->i_dnet_in_ready = 1;
  d->i_anet_in_ready = 1;
  d->i_dnet_valid = 0;
  d->i_anet_valid = 0;
  d->i_cfg_we = 0;

  tick(d); tick(d); d->i_rst = 0;

  // Configure route slot 0 -> (dst_core=1, dst_slot=0).
  d->i_cfg_we = 1; d->i_cfg_slot = 0; d->i_cfg_dst_core = 1; d->i_cfg_dst_slot = 0;
  tick(d);
  d->i_cfg_we = 0;

  // Run until the OUT injects a DATA flit (o_dnet_valid), or time out.
  int guard = 0;
  while (!d->o_dnet_valid && guard++ < 50) tick(d);
  assert(d->o_dnet_valid && "core never injected a DATA flit");

  // Check the emitted flit.
  assert(d->o_dnet_dst == 1 && "wrong destination core");
  assert(flit_dcore(d->o_dnet_flit) == 1 && "flit dst_core wrong");
  assert(flit_dslot(d->o_dnet_flit) == 0 && "flit dst_slot wrong");
  assert(flit_src(d->o_dnet_flit) == 2 && "flit src_core wrong");
  assert(flit_word(d->o_dnet_flit) == 65 && "flit word wrong");

  // The writer must stall (PC frozen) until the ACK returns.
  unsigned pc_stalled = d->core->u_processor->pc_q;
  tick(d); tick(d);
  assert(d->core->u_processor->pc_q == pc_stalled && "PC advanced before ACK");

  // Return the ACK; the writer should then unblock and advance.
  d->i_anet_valid = 1;
  tick(d);
  d->i_anet_valid = 0;
  tick(d);
  assert(d->core->u_processor->pc_q != pc_stalled && "core did not unblock after ACK");

  printf("core_tb PASS\n");
  delete d;
  return 0;
}
