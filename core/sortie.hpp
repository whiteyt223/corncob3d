#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Stages: 0 playing, 1 caller must run original drawnear once, 2 write config,
// 3 write result bytes DS:01e4 length140, 4 flush tiles then write world, 5 done.
// Call reset on a fresh sortie; continuation calls are inert in other stages.
void cc_sortie_reset();
uint32_t cc_sortie_stage();
void cc_sortie_requests(); // CS4906..49d7: chute, L/F1/F6/F5/F3/F2/H + frame7 popup
uint32_t cc_sortie_beep();
uint32_t cc_sortie_take_beep(); // consume a source request once across modal platform steps // last requests: 1 means original F5 700Hz/25ms beep
uint32_t cc_sortie_frame_end(uint32_t raw_frame_ticks);
uint32_t cc_sortie_begin_exit(uint32_t raw_frame_ticks); // direct donep33, e.g. fatal crash
uint32_t cc_sortie_after_near(uint32_t raw_frame_ticks);
// Stage2: hash, write DS0AF7 length u16(DS0B2C), then unhash on IO success.
void cc_sortie_config_hash();
void cc_sortie_config_unhash();
uint32_t cc_sortie_after_config();
uint32_t cc_sortie_after_results();
uint32_t cc_sortie_after_world();
// Packed DOS time bytes low→high: hundredths, seconds, minutes, hours.
void cc_sortie_timer_reset(uint32_t packed_clock);
uint32_t cc_sortie_timer_read(uint32_t packed_clock);
uint32_t cc_sortie_add_elapsed(uint32_t packed_clock); // returns seconds added; no reset
uint32_t cc_sortie_dvel_average(); // CS185c: 0 success, 2 invalid hardware arithmetic
uint32_t cc_sortie_palette(); // last dvel average: 0..255 request, 256 no request
#ifdef __cplusplus
}
#endif
