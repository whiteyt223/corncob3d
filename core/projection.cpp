#include <stdint.h>

// F3DVEC.ASM f3dpt, stopping at drawpt (hardware rasterization is separate).
// Inputs: depth, horizontal, vertical, zxpln, zypln, wxctr, wyctr.
// Outputs preserve partial writes, as the assembly does. Status: 0 reaches
// drawpt, 1 rejects near/behind point, 2 represents an x86 division exception.
namespace {
uint16_t input[7], output[2];
int32_t s16(uint16_t x) { return x<32768 ? int32_t(x):int32_t(x)-65536; }
}
extern "C" uint16_t* cc_projection_input() { return input; }
extern "C" uint16_t* cc_projection_output() { return output; }
extern "C" uint32_t cc_project_point() {
    const uint16_t z=input[0];
    if (z&32768) return 1;
    for (unsigned i=0;i<2;++i) {
        const uint16_t plane=input[3+i];
        if (!(uint16_t(plane-z)&32768)) return 1;
        if (!z) return 2;
        const int32_t numerator=s16(uint16_t(0u-input[1+i]))*s16(plane);
        const int32_t q=numerator/s16(z);
        if (q < -32768 || q > 32767) return 2;
        output[i]=uint16_t(uint32_t(q)+input[5+i]);
    }
    return 0;
}
