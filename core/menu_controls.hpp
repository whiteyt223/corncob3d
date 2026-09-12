#pragma once
#include <stdint.h>
// controls[0..2] are MOAG words2A78,2A7A,2A7C.
// kinds: keyboard0, joystick1, force-recenter2, automatic3, manual4.
extern "C" void cc_menu_controls_choice(uint16_t* controls,uint32_t kind);
// startup globals bits g=1 i=2 j=4 k=8.
extern "C" uint32_t cc_menu_controls_flags(const uint16_t* controls,uint32_t invincible);
