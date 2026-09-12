#pragma once
#include <stdint.h>
extern "C" uint32_t cc_towers_generate(const uint8_t* file,uint32_t size,uint32_t offset);
extern "C" uint8_t* cc_towers_text();
extern "C" uint32_t cc_towers_text_size();
// Original formatter/group/sort test interfaces operate on 17-byte records.
extern "C" uint32_t cc_towers_format(const uint8_t* object,int32_t x,int32_t y);
extern "C" uint32_t cc_towers_group(uint8_t* objects,uint32_t count);
// Viewer returns low16 top row; bit16 closes; bit17 is source beep;
// high byte is the original return character/value when closing.
extern "C" uint32_t cc_towers_view_key(uint32_t top,uint32_t lines,uint32_t key);
// Original D74E text writer, normal color67h, highlighted6Fh, mnemonic6Eh.
extern "C" uint32_t cc_towers_markup(const uint8_t* text,uint32_t width,uint8_t* cells);
extern "C" const char* cc_towers_field_name(uint32_t field,uint32_t part);
// Source EF63 writes X/Y in the last12 file bytes, retaining existing Z.
extern "C" uint32_t cc_towers_start(uint8_t* file,uint32_t size,uint32_t field);
// Source ED90 derives the selected field from the saved tail position.
// Returns signed word result; all valid source field positions yield0..8.
extern "C" int32_t cc_towers_initial(const uint8_t* file,uint32_t size,uint32_t single);
extern "C" uint32_t cc_towers_airfield_key(uint32_t field,uint32_t single,uint32_t key);

extern "C" uint32_t cc_tu_generate(const uint8_t*,uint32_t,uint32_t);
extern "C" uint32_t cc_tu_view_key(uint32_t top,uint32_t lines,uint32_t key);
