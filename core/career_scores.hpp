#pragma once
#include <stdint.h>
extern "C" {
// The fresh original pilot.hsc at initial process startup is 5040 zero bytes.
int32_t cc_career_scores_new(uint8_t* scores,uint32_t size);
// Clear a confirmed list 0=missions,1=careers,2=fastest: name byte0 only.
int32_t cc_career_scores_clear(uint8_t* scores,uint32_t size,uint32_t list);
// Original comparison of two 134-byte scored result/totals records; -1,0,+1.
int32_t cc_career_scores_compare(const uint8_t* a,const uint8_t* b);
// Original fastest comparison: 623-byte career against 162-byte score row.
int32_t cc_career_scores_fast_compare(const uint8_t* pilot,const uint8_t* row);
// Already accepted/accumulated career. Reads last result+1c and totals+1ae.
// Returns bitmask of lists written (bits0,1,2), -1 invalid input.
// If pilot active theater byte+23d has bit80, label is Training Mission.
// Otherwise supply original cached theater name (up to27 bytes, excluding NUL).
// Original strcpy into22-byte theater label overwrites the result on long names.
int32_t cc_career_scores_update(uint8_t* scores,uint32_t size,const uint8_t* pilot,uint32_t trainee,const uint8_t* theater_name,uint32_t theater_name_size);
}
