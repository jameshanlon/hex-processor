#include <verilated.h>
#include "Vlink_interface.h"
#include <cassert>
#include <cstdio>
#include <cstdint>

// data_flit_t packed layout (38 bits total, MSB..LSB):
//   [37:36] dst_core  (2 bits)
//   [35:34] dst_slot  (2 bits)
//   [33:32] src_core  (2 bits)
//   [31:0]  word      (32 bits)
//
// Verilator exposes this as QData (uint64_t) since it is 33-64 bits.
//
// ack_flit_t packed layout (2 bits):
//   [1:0] dst_core
// Exposed as CData (uint8_t).

static uint64_t make_dnet_flit(uint32_t dst_core, uint32_t dst_slot,
                                uint32_t src_core, uint32_t word) {
    return ((uint64_t)(dst_core & 0x3) << 36)
         | ((uint64_t)(dst_slot & 0x3) << 34)
         | ((uint64_t)(src_core & 0x3) << 32)
         | (uint64_t)word;
}

static uint32_t flit_word(uint64_t f)     { return (uint32_t)(f & 0xFFFFFFFF); }
static uint32_t flit_src_core(uint64_t f) { return (uint32_t)((f >> 32) & 0x3); }
static uint32_t flit_dst_slot(uint64_t f) { return (uint32_t)((f >> 34) & 0x3); }
static uint32_t flit_dst_core(uint64_t f) { return (uint32_t)((f >> 36) & 0x3); }

// One clock edge.
static void tick(Vlink_interface *d) { d->i_clk = 0; d->eval(); d->i_clk = 1; d->eval(); }

int main() {
    Vlink_interface *d = new Vlink_interface;

    // -----------------------------------------------------------------------
    // Test 1: OUT — inject DATA flit and wait for ACK.
    // Config: slot 0 -> (dst_core=2, dst_slot=1)
    // -----------------------------------------------------------------------
    {
        // Reset.
        d->i_rst           = 1;
        d->i_clk           = 0;
        d->i_core_id       = 0;
        d->i_op_out        = 0;
        d->i_op_in         = 0;
        d->i_slot          = 0;
        d->i_areg          = 0;
        d->i_cfg_we        = 0;
        d->i_dnet_in_ready = 0;
        d->i_dnet_valid    = 0;
        d->i_dnet_flit     = 0;
        d->i_anet_in_ready = 0;
        d->i_anet_valid    = 0;
        d->i_anet_flit     = 0;
        tick(d); tick(d);
        d->i_rst = 0;

        // Config: slot 0 -> (dst_core=2, dst_slot=1).
        d->i_cfg_we        = 1;
        d->i_cfg_slot      = 0;
        d->i_cfg_dst_core  = 2;
        d->i_cfg_dst_slot  = 1;
        tick(d);
        d->i_cfg_we = 0;

        // Drive OUT op on slot 0, areg=0x55.
        d->i_op_out        = 1;
        d->i_slot          = 0;
        d->i_areg          = 0x55;
        d->i_dnet_in_ready = 1;  // network ready immediately
        tick(d);  // IDLE->OUT_SEND transition; outputs live from now

        // FSM is now in OUT_SEND; combinational outputs valid.
        d->eval();
        assert(d->o_busy       && "Test1: should be busy in OUT_SEND");
        assert(d->o_dnet_valid && "Test1: o_dnet_valid should be high");
        assert(flit_dst_core(d->o_dnet_flit) == 2   && "Test1: dst_core should be 2");
        assert(flit_dst_slot(d->o_dnet_flit) == 1   && "Test1: dst_slot should be 1");
        assert(flit_src_core(d->o_dnet_flit) == 0   && "Test1: src_core should be 0");
        assert(flit_word(d->o_dnet_flit)     == 0x55 && "Test1: word should be 0x55");
        assert(d->o_dnet_dst == 2 && "Test1: o_dnet_dst should be 2");

        // Tick: i_dnet_in_ready=1 so OUT_SEND->OUT_WAIT.
        tick(d);
        d->i_dnet_in_ready = 0;

        // Drive ACK arrival.
        d->i_anet_valid = 1;
        d->i_anet_flit  = 0;  // dst_core=0 (this core)
        d->eval();
        // In OUT_WAIT with i_anet_valid: o_done pulses, o_busy drops.
        assert(d->o_done  && "Test1: o_done should pulse when ACK arrives");
        assert(!d->o_busy && "Test1: o_busy should drop when o_done");

        // Tick: OUT_WAIT->IDLE.
        tick(d);
        d->i_anet_valid = 0;
        d->i_op_out     = 0;
        printf("Test 1 (OUT inject + ACK): PASS\n");
    }

    // -----------------------------------------------------------------------
    // Test 2: IN — wait for slot, deliver word, emit ACK.
    // -----------------------------------------------------------------------
    {
        // Fresh reset.
        d->i_rst        = 1;
        d->i_op_out     = 0;
        d->i_op_in      = 0;
        d->i_dnet_valid = 0;
        d->i_anet_valid = 0;
        tick(d); tick(d);
        d->i_rst = 0;

        // Drive IN on slot 3 (buffer empty => stay IDLE, busy).
        d->i_op_in = 1;
        d->i_slot  = 3;
        d->eval();
        assert(d->o_busy  && "Test2: should be busy");
        assert(!d->o_done && "Test2: no o_done yet, buffer empty");

        tick(d);  // IDLE (rx_valid[3]=0), stays IDLE
        assert(d->o_busy  && "Test2: still busy after tick");
        assert(!d->o_done && "Test2: still no o_done");

        // Deliver DATA flit: dst_slot=3, src_core=1, word=0x99.
        // o_dnet_out_ready should be high (slot 3 buffer empty) when valid is driven.
        d->i_dnet_valid = 1;
        d->i_dnet_flit  = make_dnet_flit(/*dst_core=*/0, /*dst_slot=*/3,
                                          /*src_core=*/1, /*word=*/0x99);
        d->eval();
        assert(d->o_dnet_out_ready && "Test2: o_dnet_out_ready should be high (slot 3 empty)");
        tick(d);
        // Flit accepted on this clock edge: rx_valid[3] set.
        // FSM was IDLE and checked rx_valid[3] on this posedge — still 0 (registered).
        // So FSM stays IDLE this tick; need one more tick to see rx_valid[3]=1 -> IN_ACK.
        d->i_dnet_valid = 0;
        tick(d);  // IDLE sees rx_valid[3]=1, transitions to IN_ACK.

        // Now in IN_ACK.
        d->eval();
        assert(d->o_anet_valid         && "Test2: o_anet_valid should be high in IN_ACK");
        assert(d->o_anet_dst == 1      && "Test2: o_anet_dst should be 1");
        assert((d->o_anet_flit & 0x3) == 1 && "Test2: ACK flit dst_core should be 1");
        assert(!d->o_done              && "Test2: o_done not yet, anet_in_ready not set");

        // Drive i_anet_in_ready; o_done pulses, o_in_word=0x99.
        d->i_anet_in_ready = 1;
        d->eval();
        assert(d->o_done             && "Test2: o_done should pulse on anet_in_ready");
        assert(!d->o_busy            && "Test2: o_busy should drop with o_done");
        assert(d->o_in_word == 0x99  && "Test2: o_in_word should be 0x99");

        tick(d);
        d->i_anet_in_ready = 0;
        d->i_op_in         = 0;
        printf("Test 2 (IN wait + deliver + ACK): PASS\n");
    }

    // -----------------------------------------------------------------------
    // Test 3: Per-slot independence — deliver to slot 1 while waiting on slot 0.
    // -----------------------------------------------------------------------
    {
        // Fresh reset.
        d->i_rst           = 1;
        d->i_op_out        = 0;
        d->i_op_in         = 0;
        d->i_dnet_valid    = 0;
        d->i_anet_valid    = 0;
        d->i_anet_in_ready = 0;
        tick(d); tick(d);
        d->i_rst = 0;

        // Drive IN on slot 0 (buffer empty => stay IDLE, busy).
        d->i_op_in = 1;
        d->i_slot  = 0;
        tick(d);  // IDLE, rx_valid[0]=0, stays IDLE
        assert(d->o_busy  && "Test3: busy waiting on slot 0");
        assert(!d->o_done && "Test3: no done for slot 0");

        // Deliver a DATA flit for slot 1 (different from the awaited slot 0).
        // o_dnet_out_ready should be high (slot 1 buffer empty) when valid is driven.
        d->i_dnet_valid = 1;
        d->i_dnet_flit  = make_dnet_flit(/*dst_core=*/0, /*dst_slot=*/1,
                                          /*src_core=*/2, /*word=*/0xAB);
        d->eval();
        assert(d->o_dnet_out_ready && "Test3: ready to accept flit for slot 1");
        tick(d);
        // Flit accepted into slot 1 buffer; FSM still IDLE (slot 0 still empty).
        d->i_dnet_valid = 0;

        // Slot 0 IN still waiting; no spurious done or ACK.
        d->eval();
        assert(d->o_busy        && "Test3: still busy waiting on slot 0");
        assert(!d->o_done       && "Test3: slot 0 still not filled");
        assert(!d->o_anet_valid && "Test3: no ACK emitted (slot 0 not filled)");

        printf("Test 3 (per-slot independence): PASS\n");
    }

    printf("liu_tb PASS\n");
    delete d;
    return 0;
}
