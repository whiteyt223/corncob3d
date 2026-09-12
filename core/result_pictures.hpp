#pragma once
#include <stdint.h>
// Eight-byte plan: count, final grflag, switch-to-text, wait-before bit mask,
// then three picture numbers and a zero reserved byte. Unused picture IDs=255.
extern "C" uint32_t cc_result_picture_sequence(uint32_t status, uint32_t death,
    uint32_t friendly_destroyed, uint32_t grflag, uint8_t* plan);
// Same plan using original DS fields; reflects grflag/pictnumber side effects.
// A nonzero cecode skips the complete PICT routine, as 3.ASM does.
extern "C" uint32_t cc_result_picture_plan(uint8_t* ds, uint32_t cecode, uint8_t* plan);
