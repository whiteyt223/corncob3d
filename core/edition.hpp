#pragma once
#include <stdint.h>
// Process/session profile; zero is the unchanged shareware default.
extern "C" uint32_t cc_edition_set(uint32_t edition);
extern "C" uint32_t cc_edition_get();
extern "C" uint32_t cc_edition_is_deluxe();
extern "C" uint32_t cc_edition_is_other_worlds();
