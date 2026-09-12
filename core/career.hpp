#pragma once
#include <stdint.h>
// Original MOAG 3.42 byte records. No host pointers or platform time in records.
// Metadata is 28 bytes: LE16 seed,option2a78,option2a7c,mixed2a4c at0,2,4,6;
// preferenceB4 at8, reserved9..11; LE32 oldTime,savedTime,checksum,opaque at12,16,20,24.
// Negative result codes: -1 bad argument/format, -2 short output, -3 name already used,
// -4 roster full. File APIs require separate input/output buffers.
extern "C" {
uint32_t cc_career_file_size(uint32_t count);
int32_t cc_career_encode(const uint8_t* metadata,const uint8_t* pilots,uint32_t count,uint8_t* out,uint32_t capacity);
int32_t cc_career_decode(const uint8_t* file,uint32_t size,uint8_t* metadata,uint8_t* pilots,uint32_t pilot_capacity);
int32_t cc_career_active(const uint8_t* pilots,uint32_t count);
int32_t cc_career_name_gate(const uint8_t* pilots,uint32_t count,const uint8_t* name,uint32_t name_size);
int32_t cc_career_new(uint8_t* pilot,const uint8_t* name,uint32_t name_size,uint32_t trainee);
// Select gate:0 allowed,1 stockade/flag1,2 resurrection offer. Original Activate
// Pilot offers resurrection for flags0xa before its stockade message; explicit
// Resurrect Pilot menu instead checks the clock first (see resurrect function).
int32_t cc_career_select_gate(const uint8_t* pilot,uint32_t now);
// selected=-1 performs the original manual deactivation: it clears old active
// bit and leaves the trainee flags unchanged. after_sortie has different behavior.
int32_t cc_career_activate(uint8_t* pilots,uint32_t count,uint8_t* trainee,int32_t previous,int32_t selected);
// Returns1 if penalty applied; caller skips ordinary medals/ranks on that result.
int32_t cc_career_stockade(uint8_t* pilot,uint32_t now);
// explicit_menu=1 checks clock first;0 is already-confirmed Activate Pilot offer.
// Returns0 changed,1 clock blocked,2 no killed/captured flag.
int32_t cc_career_resurrect(uint8_t* pilot,uint32_t now,uint32_t explicit_menu);
// Explicit-menu caller applies this to every successfully read THT pilot record.
int32_t cc_career_resurrect_theater(uint8_t* theater_pilot);
// Run after reports, including discarded sorties. Return1 switches active to trainee.
int32_t cc_career_after_sortie(uint8_t* pilot,uint8_t* trainee,uint32_t is_trainee,uint32_t now);
// Fresh THT pilot = zero record plus career name and rank; header = exact definition.
int32_t cc_career_new_theater(const uint8_t* career,const uint8_t* definition,uint8_t* header,uint8_t* theater_pilot);
int32_t cc_career_link_theater(uint8_t* career,uint32_t allocated_file_id);
int32_t cc_career_select_theater(uint8_t* career,uint32_t index);
}

extern "C" int32_t cc_career_decode_mode(uint32_t other_worlds,const uint8_t* file,uint32_t size,uint8_t* metadata,uint8_t* pilots,uint32_t pilot_capacity);
