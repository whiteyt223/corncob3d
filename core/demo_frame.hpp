#pragma once
#include <stdint.h>
extern "C" {
// The caller keeps imported bytes alive for the whole sortie. Reset before
// creating cached memory views if the browser grows Wasm memory for the file.
void cc_demo_frame_reset(const uint8_t* bytes,uint32_t size,uint32_t request);
void cc_demo_frame_begin();
uint32_t cc_demo_frame_mode();
void cc_demo_frame_timing();
uint32_t cc_demo_frame_before_refresh();
// Return3 is the ejected EOF cross-jump: rebuild normal observer matrices,
// call this tail again, and copy observer angles into effective angles.
uint32_t cc_demo_frame_record_tail();
// Additive flight_lifecycle.cpp entry; ordinary refresh APIs remain unchanged.
uint32_t cc_aircraft_refresh_frame();
// [output count,event mask,input cursor,input size,record sequence,host error]
// Events:1 open-write,2 close-write,4 open-read,8 close-read,16 original EOF.
uint32_t* cc_demo_frame_io();
uint8_t* cc_demo_frame_output();
// Call immediately before the existing throttle/crash/walk sequence. Captures
// original AH inherited by the playback flap path; no simulation DS writes.
uint32_t cc_demo_frame_controls(uint32_t plus,uint32_t forward,uint32_t backward);
uint32_t cc_demo_flap_input();
uint32_t cc_demo_bomb_input();
void cc_demo_missile_request();
uint32_t cc_demo_weapon_playback();
void cc_demo_eject_input();
void cc_demo_chute_input();
}
