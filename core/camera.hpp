#pragma once
#include <stdint.h>
extern "C" {
uint32_t cc_camera_prepare(); // CS2937..29D2, before modhr/horizon;0 ok,2 arithmetic
void cc_view_input(); // CS4110..41C6, late NumLock/Ctrl+arrow selection
void cc_view_remote_input(); // CS454C..455D, CapsLock release; after weapons
uint32_t cc_camera_towards(uint32_t target); // drawtodxvec:0 success,1 too far,2 arithmetic
uint32_t cc_camera_after_world(); // after remote inset; target facing then halt movement/fade
uint32_t cc_camera_palette(); // last after_world requested palette0..255, or256 for no request
}
