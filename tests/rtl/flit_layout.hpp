#ifndef HEX_RTL_FLIT_LAYOUT_HPP
#define HEX_RTL_FLIT_LAYOUT_HPP

#include <cstdint>

// data_flit_t packed layout (38 bits, MSB..LSB), exposed by Verilator as a
// 64-bit QData:
//   [37:36] dst_core  [35:34] dst_slot  [33:32] src_core  [31:0] word
// ack_flit_t is just [1:0] dst_core (CData).
//
// Shared by the link-interface and core testbenches so the bit offsets live in
// one place.

inline uint64_t make_dnet_flit(uint32_t dst_core, uint32_t dst_slot,
                               uint32_t src_core, uint32_t word) {
  return (static_cast<uint64_t>(dst_core & 0x3) << 36) |
         (static_cast<uint64_t>(dst_slot & 0x3) << 34) |
         (static_cast<uint64_t>(src_core & 0x3) << 32) |
         static_cast<uint64_t>(word);
}

inline uint32_t flit_word(uint64_t f) { return static_cast<uint32_t>(f); }
inline uint32_t flit_src_core(uint64_t f) { return (f >> 32) & 0x3; }
inline uint32_t flit_dst_slot(uint64_t f) { return (f >> 34) & 0x3; }
inline uint32_t flit_dst_core(uint64_t f) { return (f >> 36) & 0x3; }

#endif // HEX_RTL_FLIT_LAYOUT_HPP
