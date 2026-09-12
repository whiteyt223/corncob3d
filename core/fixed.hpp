#pragma once
#include <stdint.h>
namespace cc {
inline int32_t w(int64_t v){const uint16_t u=uint16_t(v);return u<32768?int32_t(u):int32_t(u)-65536;}
inline int32_t d(int64_t v){const uint32_t u=uint32_t(v);return u<0x80000000u?int32_t(u):int32_t(int64_t(u)-4294967296ll);}
inline int32_t sar(int32_t n,unsigned bits){if(!bits)return n;uint32_t u=uint32_t(n)>>bits;if(n<0)u|=(0xffffffffu<<(32-bits));return d(u);}
inline int32_t hi(int32_t n){return w(uint32_t(n)>>16);}
inline int32_t mul(int32_t n,int32_t b){return d(uint64_t(uint32_t(n))*uint32_t(w(b)));}
inline int32_t rdiv(int32_t n,int32_t divisor){
    const int32_t b=w(divisor);
    // Original library E913h does not trap for divisor zero: its unsigned
    // divider leaves the magnitude unchanged, then rounding adds one.
    if(!b)return d(int64_t(n)+(n<0?-1:1));
    uint32_t a=n<0?0u-uint32_t(n):uint32_t(n),m=b<0?uint32_t(-b):uint32_t(b);
    uint32_t q=a/m+((a%m)*2>=m?1:0);return d((n<0)!=(b<0)?0u-q:q);
}
inline int32_t clamp(int32_t n,int32_t low,int32_t high){return n<low?low:n>high?high:n;}
inline int32_t absw(int32_t n){return n<0?w(-n):n;}
inline int32_t hm(int32_t a,int32_t b){return w(uint32_t(w(a)*w(b))>>15);}
inline int32_t add_sat(int32_t a,int32_t b){return clamp(a+b,-32767,32767);}
}
