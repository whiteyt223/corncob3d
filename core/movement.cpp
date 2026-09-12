#include <stdint.h>
namespace {
uint32_t input[12],output[3];
int32_t s16(uint32_t x){x&=65535;return x<32768?int32_t(x):int32_t(x)-65536;}
int32_t floor_div(int32_t n,int32_t d){return n>=0?n/d:-int32_t((uint32_t(-n)+uint32_t(d)-1)/uint32_t(d));}
int32_t rounded(int32_t n,int32_t d){
    const bool negative=(n<0)!=(d<0);
    const uint32_t a=n<0?uint32_t(-n):uint32_t(n),b=d<0?uint32_t(-d):uint32_t(d);
    const int32_t q=int32_t(a/b+((a%b)*2>=b?1:0));return negative?-q:q;
}
}
extern "C" uint32_t* cc_movement_input(){return input;}
extern "C" uint32_t* cc_movement_output(){return output;}
// 3.ASM moveme + divbycl; 3DMAC.INC dwtadj; original divrs_dw.
// Inputs: position[3], velocity[3], acceleration[3], ofrmticks, ticksf,
// obj5dbits. Output: updated 32-bit position. No force/flight model implied.
extern "C" uint32_t cc_move(){
    const int32_t ticks=s16(input[9]),scale=s16(input[10]);
    if(!scale)return 2;
    const int divisor=input[11]&0x2000?4:16;
    for(unsigned i=0;i<3;++i){
        const int32_t midpoint=s16(uint32_t(floor_div(s16(input[6+i]),2))+input[3+i]);
        const int32_t adjusted=rounded(midpoint*ticks,scale);
        output[i]=input[i]+uint32_t(floor_div(adjusted,divisor));
    }
    return 0;
}
