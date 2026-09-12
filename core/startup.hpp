#pragma once
#include <stdint.h>
// All pointers address byte buffers; DS must contain 65536 bytes.
extern "C" {
void cc_startup_seed(uint8_t* ds,const uint8_t* original_ds,uint32_t ds_selector);
// Original rand_init reads the BIOS tick dword at physical 0x46c and sets bit0.
void cc_startup_random_seed(uint8_t* ds,uint32_t bios_ticks);
// MOAG e00c..e0ad. globals bits:0 skip center(g),1 override invincibility(i),
// 2 force center(j),3 keyboard(k). Ace is original pilot byte+0x25e.
// Writes original common grouped flag argument, empty if no flags. Returns length.
uint32_t cc_startup_common_flags(const uint8_t* options,uint32_t globals,uint32_t ace,uint8_t* output);
// Apply original generated pilot options to cold DS, including implicit process -s.
// Selected theater ignores options[0..6] and requests -r; chain requests -h.
void cc_startup_pilot_options(uint8_t* ds,const uint8_t* options,uint32_t globals,uint32_t ace,uint32_t selected_theater,uint32_t chain);
// Original 52-byte 3D.CFG decode/load. No calibration UI or registration bypass.
void cc_startup_load_cfg(uint8_t* ds,const uint8_t* encoded_cfg);
// After options/CFG and before world read: numlock=joyflag, original initial
// startcoords -> observer/effective observer, onrunway/closetower, objmov params.
// Then caller runs original objmov(player), ground initialization and matrices.
void cc_startup_before_world(uint8_t* ds);
// After selected-theater world read; 0 success. Generated mode sets newguyflag0
// and returns -1: call original initperms, then generated_observer and recalcmats.
int32_t cc_startup_after_world(uint8_t* ds);
void cc_startup_generated_observer(uint8_t* ds);
// Seven parsed signed 32-bit X-line numbers, low words consumed by original.
// valid_last reports original successful seventh dec_to_dwords conversion.
void cc_startup_theater_params(uint8_t* ds,const int32_t* values,uint32_t valid_last);
// After readcolors, reproduces original dark-sky override using DS skycolors.
void cc_startup_sky_detail(uint8_t* ds);
// Original mode10 BIOS palette supplied as16 records [DAC index,R6,G6,B6].
// Then unpackbits/setskygnd read current CCT header colorbitsDS1fae.
void cc_startup_palette(uint8_t* ds,const uint8_t* bios_palette64);
// Original gscum layout4566bytes: countword0; 14-byte records at2,0x47e,0x8fa.
// Copy pristine tables, run startup generation, update shared DS RNG.
uint8_t* cc_startup_ground_buffer();
void cc_startup_ground(uint8_t* ds,const uint8_t* original_tables4566);
// Link startup_matrices.cpp to existing original integer math/vector exports.
void cc_startup_matrices(uint8_t* ds);
// Original recalcmats1f68; generated branch invokes this after yaw0.
// Updates only orot/norot and original trig/negflg scratch, preserves plane mats.
void cc_startup_recalcmats(uint8_t* ds);
}

extern "C" void cc_startup_theater_params_mode(uint8_t* ds,const int32_t* values,uint32_t valid_last,uint32_t registered);
