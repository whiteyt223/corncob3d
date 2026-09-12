#pragma once
#include <stdint.h>
extern "C" {
// Original initperms: consumes shared DS/RNG, creates active records flagged
// 8000 for expmov. Returns the final strtperm carry (0 success,1 no slot).
uint32_t cc_initperms();
// Diagnostic requested / created / failed object allocations.
uint32_t* cc_generated_world_result();
}
