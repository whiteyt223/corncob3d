#pragma once
#include <stdint.h>
extern "C" {
uint8_t* cc_world_file_buffer();
uint32_t cc_world_file_capacity();
// Buffer offset0 is CCT/3univ.dat; offset671 is the world in original THT.
// Read leaves existing allocation bytes not supplied by the file untouched.
// Write applies original type-word XOR IN PLACE and leaves world encoded.
// Status0 success,1 original header failure,2 external buffer/storage bound.
uint32_t cc_world_read_file(uint32_t size,uint32_t offset);
uint32_t cc_world_write_file(uint32_t offset);
// result = status, end file offset, completed tiles, records processed.
uint32_t* cc_world_io_result();
// Original successful allocatemem side effects; pass caller's virtual DOS
// segment word. Clears only0x45f80 of0x45f90 bytes, preserving final16.
void cc_world_allocate(uint32_t memSegment);
// Original cleanexp: flags/code only, plus original resetnplaced.
void cc_world_clean_objects();
// Original dotiles, including invalid coordinate DS writes; call after
// startup observer setup, at the existing frame's tile-update stage.
uint32_t cc_world_update_tiles();
}

extern "C" uint32_t cc_world_read_file_mode(uint32_t size,uint32_t offset,uint32_t plain);
extern "C" uint32_t cc_world_write_file_mode(uint32_t offset,uint32_t plain);
