#pragma once
#include <stdint.h>
// Other Worlds actual 3.EXE demo state, 56 bytes, all little endian:
// 0 flags; 1 effective XYZ (3 i32); 13 angles (3 u16); 19 deltas (3 i32);
// 31 acceleration (3 i16); 37 mode (0 idle,1 record,255 playback);
// 38 max acceleration (u16); 40 diagnostic saved deltas (12);52 repeat;53..55 spare.
// Shareware and Deluxe have no codec or demo file entry; enable only for OW.
extern "C" {
uint32_t cc_demo_compact(uint8_t* state,const uint8_t* observer,uint8_t* output);
// Returns consumed bytes<<16 | original carry<<8 | source error (0 or25).
// FFFFFFFF is a host truncated-buffer error, with no mutation. The original IO
// primitive's stale bytes on short DOS reads are not fabricated here.
uint32_t cc_demo_decode(uint8_t* state,const uint8_t* input,uint32_t size,uint32_t ejected);
void cc_demo_update(uint8_t* state,const uint8_t* observer,uint32_t frame);
// Returns0 success,1 original signed-IDIV overflow. Partial writes match CPU.
uint32_t cc_demo_apply(const uint8_t* state,uint8_t* observer,uint8_t* velocity,uint32_t ticksf,uint32_t ejected);
// Call after update at original QWRITEDEMO. scan points to source scan byte.
// output capacity must be74. Return emitted bytes | (1 open / 2 close)<<16.
// An open/write/close failure is an IO boundary; see handoff for error49 behavior.
uint32_t cc_demo_record(uint8_t* state,const uint8_t* observer,uint8_t* scan,uint8_t* output);
void cc_demo_merge(uint8_t* state,uint32_t previous_flags);
uint32_t cc_demo_ticks(const uint8_t* state,uint32_t normal_ticks);
}
