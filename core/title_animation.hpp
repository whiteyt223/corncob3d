#pragma once
#include <stdint.h>
extern "C" {
uint8_t* cc_title_data();
uint32_t cc_title_data_capacity();
void cc_title_init(uint32_t music, uint32_t sound, uint32_t seed);
uint32_t cc_title_step(int32_t key, uint32_t music_playing);
uint32_t cc_title_phase();
uint8_t* cc_title_pixels();
uint8_t* cc_title_palette();
uint8_t* cc_title_video();
uint8_t* cc_title_events();
uint32_t cc_title_event_count();
uint16_t* cc_title_locals();
}

extern "C" void cc_title_init_profile(uint32_t profile,uint32_t music,uint32_t sound,uint32_t seed);
