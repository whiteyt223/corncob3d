#pragma once
#include <stdint.h>
extern "C" {
uint8_t* cc_airfield_summary_buffer(); // original nine 14-byte records: x32,y32,planes16,objectives16,unused16
uint32_t* cc_airfield_summary_result(); // remaining16,destroyed16,oneAirfield flag,status
uint32_t cc_airfield_analyze_file(const uint8_t* file,uint32_t size,uint32_t offset); // raw CCT offset0; THT offset671
}

extern "C" uint32_t cc_airfield_analyze_file_mode(const uint8_t* file,uint32_t size,uint32_t offset,uint32_t plain);
