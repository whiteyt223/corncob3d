#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Pure decisions copied from original 3.EXE/MOAG/TOWER paths; no UI or file IO.
// Return 0 no request, 1 ready to relocate, 2 too damaged.
uint32_t cc_flow_shortcut_gate(uint32_t pending, uint32_t damage);
// original low-byte flags: radio, close-to-tower, already-warned, ejected.
// Return 0 exit sortie, 1 airborne warning, 2 on-foot warning.
uint32_t cc_flow_escape(uint32_t radio, uint32_t close, uint32_t warned, uint32_t ejected);
// flags: abort=1, invulnerable=2, tester=4, exitAlready=8,
// home=16, neverLeft=32, rescued=64. Does not produce landing/crash bits.
uint32_t cc_flow_finalize_status(uint32_t status, uint32_t flags);
// Returns homeNow low byte (also homeflag), neverLeft in next byte.
uint32_t cc_flow_home(uint32_t homeNow, uint32_t close, uint32_t neverLeft, uint32_t frame);
// MOAG: option byte +7 bit0, override DS:00ae, error DS:ab30.
// Return 0 ordinary commit, 1 discard invulnerable, 2 ask error choice,
// 3 discard error choice, 4 discard abort. choice: 0 undecided, 1 accept, 2 decline.
uint32_t cc_flow_commit_gate(uint32_t status, uint32_t optionByte7, uint32_t overrideWord, uint32_t engineError, uint32_t choice);
uint32_t cc_flow_career_flags(uint32_t careerFlags, uint32_t resultStatus);
// plane decrement uses requestedExtraPlane DS:b1b3, NOT nplaneslost.
// Three uint16_t fields: planes remaining, objectives remaining, destroyed.
void cc_flow_theater_totals(uint16_t *totals, uint32_t requestedExtraPlane, uint32_t objectives);
// Case-sensitive original TOWER marker extraction; marker itself is omitted.
// Returns payload bytes incl NUL, 0 not a marker, -1 output too small/invalid.
int32_t cc_flow_start_marker(const uint8_t *line, uint32_t length, uint8_t *out, uint32_t capacity);
// Bounds-checked MOAG.CFG prefix lookup. Returns 0 success; 1 bad data.
// out = named count, signed active index bits, active file offset, trainee offset.
uint32_t cc_flow_cfg_active(const uint8_t *cfg, uint32_t size, uint32_t *out);
// Original THT byte layout [48 header][623 pilot][world bytes].
// Returns required/written byte count; 0 invalid/short output. No IO.
uint32_t cc_flow_encode_theater(const uint8_t *header, const uint8_t *pilot,
 const uint8_t *world, uint32_t worldSize, uint8_t *out, uint32_t capacity);
#ifdef __cplusplus
}
#endif
