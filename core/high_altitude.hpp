#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Original CS3FFF..404E. Stages0 no transition,1 flush tiles,2 drawnear,
// 3 erase needles/flip page/erase needles,4 run SRATS child presentation,
// 5 sortie finalization prepared (continue using cc_sortie_stage()).
void cc_high_altitude_reset();
uint32_t cc_high_altitude_stage();
uint32_t cc_high_altitude_begin();
uint32_t cc_high_altitude_after_flush();
uint32_t cc_high_altitude_after_near();
uint32_t cc_high_altitude_after_presentation(uint32_t packed_clock);
uint32_t cc_high_altitude_after_stars(uint32_t packed_clock,uint32_t exec_failed,uint32_t raw_frame_ticks);
#ifdef __cplusplus
}
#endif
