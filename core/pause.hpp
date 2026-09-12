#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Exactly once at original CS260C before queued radio/tower/pause handlers.
// Returns DS ofrmticks; qtimeinc adds directly to raw_ticks, not the ISR.
uint32_t cc_frame_input_prefix(uint32_t raw_frame_ticks);
// Original CS2784..279D calibration equality guard; once after pause, before map.
void cc_frame_after_pause();
// Original pause/help CS26A3..2784. Stages0 inactive,1 pause poll,
// 2 load requested help bitmap. Raw/legacy clocks continue, simulation waits.
void cc_pause_reset();
uint32_t cc_pause_stage();
uint32_t cc_pause_begin(uint32_t packed_DOS_clock);
uint32_t cc_pause_update(uint32_t packed_DOS_clock); // one original waitforp iteration
uint32_t cc_pause_image_id();
uint8_t* cc_pause_image_buffer(); // host supplies640*350 decoded indexed pixels
// Success copies only original top199rows to both pages. Failure sets error40.
uint32_t cc_pause_after_image(uint32_t failed,uint32_t packed_DOS_clock);
uint32_t cc_pause_flash_ax(); // original register retained across pause loops
#ifdef __cplusplus
}
#endif
