#pragma once
#include <stdint.h>
extern "C" {
// Original14-byte gscum point. star applies original signed /256 before
// range reduction. Returns original last-view carry, or2 for IDIV failure.
uint32_t cc_g3dobj(const uint8_t* record,uint32_t star);
// Runtime gscum4566bytes; advance enables original failed-point regeneration
// and per-point RNG. Returns0 success,2 arithmetic,3 invalid count.
uint32_t cc_ground_frame(uint8_t* tables,uint32_t advance);
uint8_t* cc_ground_buffer(); //4566-byte browser packing buffer
uint32_t* cc_ground_stats(); // attempted, rejected; reset per ground_frame
void cc_ground_input(); // original G-releaseA2 after crash, before pilot walk
void cc_world_horizon(); // original modhr writes DS geometry/dvflg
}
