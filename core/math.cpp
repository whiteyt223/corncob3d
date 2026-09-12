#include <stdint.h>
#include "tables.hpp"

// Source authority: SIN.ASM sin/cos/ssin/scos and ATN.ASM atn.
// Return packing for magnitude routines: low 16 bits AX; bit 16 negflg/carry.
extern "C" uint32_t cc_sin(uint32_t input) {
    uint16_t angle=static_cast<uint16_t>(input);
    if ((angle & 0x7fff)==0) return 0;
    const uint32_t negative=(angle & 0x8000) ? 1u : 0u;
    if (negative) angle=static_cast<uint16_t>(0u-angle);
    if (angle & 0x4000) angle=static_cast<uint16_t>(32768u-angle);
    const uint32_t index=angle>>6;
    const uint32_t fraction=angle&63;
    const uint32_t base=corncob::sin_table[index];
    // Original reads one word past the table at exactly pi/2 but multiplies
    // its difference by zero. Do not reproduce the unobservable memory read.
    const uint32_t interpolated=fraction ?
        ((corncob::sin_table[index+1]-base)*fraction)/64 : 0;
    return base+interpolated+(negative<<16);
}
extern "C" uint32_t cc_cos(uint32_t angle) { return cc_sin(angle+16384u); }
extern "C" int32_t cc_ssin(uint32_t angle) {
    const uint32_t value=cc_sin(angle);
    const int32_t magnitude=static_cast<int32_t>((value&65535u)>>1);
    return (value>>16) ? -magnitude : magnitude;
}
extern "C" int32_t cc_scos(uint32_t angle) { return cc_ssin(angle+16384u); }
extern "C" uint32_t cc_atn(uint32_t input) {
    const uint16_t ax=static_cast<uint16_t>(input);
    return (ax&0x8000) ? 65536u : corncob::atan_table[ax>>5];
}
// Source authority: CSQRT.ASM csqrt. Preserve its table, not its stale comment.
extern "C" uint32_t cc_csqrt(uint32_t input) {
    const uint16_t ax=static_cast<uint16_t>(input);
    return ax>=1024 ? 65536u : corncob::csqrt_table[ax];
}
