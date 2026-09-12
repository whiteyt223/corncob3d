#pragma once
#include <stdint.h>
extern "C" {
// Original SPKE; UINT32_MAX represents the original zero-divisor CPU fault.
uint32_t cc_joystick_scale(uint32_t displacement,uint32_t extent);
// Original JOYTD after four original keyboard pressure reads. keyboard is two
// signed centered words; result is original AX/BX (0..1024), X in low word.
uint32_t cc_joystick_select(uint32_t raw_x,uint32_t raw_y,uint32_t keyboard);
uint32_t cc_joystick_input(uint32_t raw_x,uint32_t raw_y);
// The chute swing helper calls JOYTD a second time in the same source frame.
// This reuses the host sample latched by input, but reads key pressure again.
uint32_t cc_joystick_read_centered();
uint32_t cc_joystick_brake(uint32_t keys); // 1 period, 2 Insert, 4 left Shift
// 0 ready, 1 calibration requested, 91 hardware missing (sets source error).
uint32_t cc_joystick_startup(uint32_t present,uint32_t raw_x,uint32_t raw_y);
void cc_joystick_calibration_begin();
// stage 0 upper-left, 1 lower-right, 2 center. Returns next stage 1/2,
// 0 complete, 3 bad value/retry, UINT32_MAX invalid host stage.
uint32_t cc_joystick_calibration_sample(uint32_t stage,uint32_t raw_x,uint32_t raw_y);
uint32_t cc_joystick_calibration_abort(); // original code 93; JINIT stays set
void cc_joystick_calibration_finish_reentry(); // original rejoyflag 1BAA clear; boss F4D survives
// Original joystick late-frame weapon branch; keys: 1 Space, 2 Shift, 4 C.
// port has original active-low bits 0x10/0x20. Configure cannons separately.
uint32_t cc_joystick_weapons(uint32_t keys,uint32_t port);
}
