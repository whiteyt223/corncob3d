#pragma once
#include <stdint.h>
extern "C" {
// 0 handled (including no contact), 1 unsupported callback, 2 invalid input.
uint32_t cc_contact_crash(uint32_t object);
uint32_t cc_contact_walk(uint32_t object);
uint32_t cc_contact_pairs(uint32_t source,uint32_t slot);
uint32_t cc_contact_home();
}
