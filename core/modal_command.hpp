#pragma once
#include <stdint.h>
// Call after cc_modal_tower_audio, except joystick recalibration requests.
// argument is original runtower AX: currenttower for F2, FFFF for boss.
extern "C" void cc_modal_tower_command(uint32_t argument);
