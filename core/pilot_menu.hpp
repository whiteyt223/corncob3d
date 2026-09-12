#pragma once
#include <stdint.h>
extern "C" {
// options points to the eight bytes at pilot+0x234.
void cc_options_select_training(uint8_t* pilot); // preserves remembered theater index low7 bits
void cc_options_context(uint8_t* options,uint32_t combat);
// kind0 flight-model,1 inflight-comments,2 combat invulnerability,3 training invulnerability.
uint32_t cc_options_toggle(uint8_t* options,uint32_t kind);
void cc_options_defaults(uint8_t* options);
// field0..2 counts(0..25),3..5 wickedness(0..8). Invalid input returns1 unchanged.
uint32_t cc_options_set_enemy(uint8_t* options,uint32_t field,uint32_t value);
// bit0 no local objectives, bit1 no local planes, bit2 theater complete.
uint32_t cc_launch_warning_flags(uint32_t active,uint32_t local_planes,uint32_t local_objectives,uint32_t total_objectives);
// kind1 no objectives,2 no planes. Returns -1 abort,0 choose another tower,
// 1 proceed,2 request an extra plane. Stores is the signed header+0x26 word.
int32_t cc_launch_warning_choice(uint32_t kind,uint32_t stores,uint32_t key);
}
