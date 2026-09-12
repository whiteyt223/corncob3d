#pragma once
#include <stdint.h>
// MOAG 3.42 management operations. Records retain all original opaque bytes.
// Score file = 10*180 mission rows, 10*162 career rows, 10*162 fastest rows.
extern "C" {
uint32_t cc_career_scores_size();
// Returns number changed; -1 invalid input. Name sizes exclude their NUL.
int32_t cc_career_scores_rename(uint8_t* scores,uint32_t size,const uint8_t* old_name,uint32_t old_size,const uint8_t* name,uint32_t name_size);
int32_t cc_career_scores_delete(uint8_t* scores,uint32_t size,const uint8_t* name,uint32_t name_size);
// Same-name rename is a duplicate (-3); full 15-pilot roster can be renamed.
// Optional scores is the complete raw 5040-byte file, or null if unavailable.
// Original rename does not rewrite THT pilot names.
int32_t cc_career_rename_pilot(uint8_t* pilots,uint32_t count,uint32_t selected,const uint8_t* name,uint32_t name_size,uint8_t* scores);
// state = LE16 count, LE16 signed active index (-1=trainee).
// pilots supplies SIXTEEN 623-byte slots, including the original trailing slot.
// If scores exists, its fastest table aliases slot15+4 as in MOAG.
// The first four bytes of slot15 remain caller supplied. No trainee flags change.
// deleted_ids receives 20 bytes (unused bytes zero), preserving duplicate IDs.
// Returns number of unlink requests, -1 invalid input. Count decreases by one.
int32_t cc_career_delete_pilot(uint8_t* pilots,uint8_t* state,uint32_t selected,uint8_t* scores,uint8_t* deleted_ids);
// Execute already-confirmed Delete Theater. Returns original byte file ID.
// Entry with completed!=0 requires Y/y; incomplete theater has no confirmation.
int32_t cc_career_close_theater(uint8_t* pilot,uint32_t selected);
// Exact cached-list unlink over original 64K DS. Head word normally DS0b44;
// node id word+0x22, next word+0x24. all=0 theater delete; all=1 pilot delete.
// No nodes are freed. Returns removed count, -1 invalid/cyclic input.
int32_t cc_career_unlink_theater_nodes(uint8_t* ds,uint32_t head_offset,uint32_t file_id,uint32_t all);
}
