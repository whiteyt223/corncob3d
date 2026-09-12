#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Independent SRATS.EXE data segment and inherited indexed EGA storage.
uint8_t* cc_stars_state();
uint8_t* cc_stars_video(); // 65536 EGA addresses ×8 pixels; copy parent VRAM before begin
void cc_stars_begin(uint32_t sky_index,uint32_t r,uint32_t g,uint32_t b,uint32_t bios_tick);
// One original BIOS-timed frame, then one queued ASCII key (0 none).
// Returns0 running,1 Escape return,2 original child exitcode1,3 arithmetic error.
uint32_t cc_stars_frame(uint32_t key_ascii,uint32_t bios_tick);
uint32_t cc_stars_status();
uint32_t cc_stars_display_page(); // existing logical video page:1=offset0,0=7e00
uint32_t* cc_stars_palette(); // sky DAC index, R,G,B; values0..63
void cc_stars_present(); // copy visible child page to shared cc_framebuffer
#ifdef __cplusplus
}
#endif
