#include <verilated.h>
#include "Vrouter.h"
#include <cassert>
#include <cstdio>

// One clock edge (low then high).
static void tick(Vrouter *d) { d->i_clk = 0; d->eval(); d->i_clk = 1; d->eval(); }

int main() {
  Vrouter *d = new Vrouter;
  d->i_rst = 1; tick(d); tick(d); d->i_rst = 0;
  // Hold output 2 stalled so the delivered flit is held (and observable), which
  // also exercises output back-pressure: the flit must not be lost or
  // overwritten while i_out_ready is low.
  d->i_out_ready = 0x0;
  // Inject a flit on input 0 addressed to output 2.
  d->i_in_valid = 0x1;
  d->i_in_dst[0] = 2;
  d->i_in_flit[0] = 0xABCD;
  tick(d);                 // load input buffer
  d->i_in_valid = 0x0;
  tick(d);                 // grant -> output register (held, output not ready)
  tick(d);                 // still held while stalled
  assert((d->o_out_valid & (1 << 2)) && "flit not delivered/held at output 2");
  assert(d->o_out_flit[2] == 0xABCD && "wrong flit payload");
  // Now drain it.
  d->i_out_ready = 0xF; tick(d);
  assert(!(d->o_out_valid & (1 << 2)) && "output 2 did not drain when ready");

  // Fairness: inputs 0 and 1 both target output 3; both must drain (no starvation).
  d->i_rst = 1; tick(d); tick(d); d->i_rst = 0;
  d->i_out_ready = 0xF;
  d->i_in_valid = 0x3;
  d->i_in_dst[0] = 3; d->i_in_flit[0] = 0x11;
  d->i_in_dst[1] = 3; d->i_in_flit[1] = 0x22;
  bool seen11 = false, seen22 = false;
  for (int i = 0; i < 8; i++) {
    tick(d);
    d->i_in_valid = 0x0; // injected once; let them flow through
    if (d->o_out_valid & (1 << 3)) {
      if (d->o_out_flit[3] == 0x11) seen11 = true;
      if (d->o_out_flit[3] == 0x22) seen22 = true;
    }
  }
  assert(seen11 && seen22 && "round-robin starved an input");

  printf("router_tb PASS\n");
  delete d;
  return 0;
}
