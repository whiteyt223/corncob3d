#pragma once
#include "fixed.hpp"
extern "C" uint8_t* cc_flight_state();
namespace cc::state {
inline uint8_t b(unsigned p){return cc_flight_state()[uint16_t(p)];}
inline uint16_t u(unsigned p){return uint16_t(b(p)|(unsigned(b(p+1))<<8));}
inline int32_t s(unsigned p){return w(u(p));}
inline int32_t l(unsigned p){return d(uint32_t(u(p))|(uint32_t(u(p+2))<<16));}
inline void wb(unsigned p,unsigned v){cc_flight_state()[uint16_t(p)]=uint8_t(v);}
inline void sw(unsigned p,int32_t v){wb(p,unsigned(v));wb(p+1,unsigned(v)>>8);}
inline void sl(unsigned p,int32_t v){sw(p,v);sw(p+2,int32_t(uint32_t(v)>>16));}
inline void addl(unsigned p,int32_t v){sl(p,d(int64_t(l(p))+v));}
inline uint16_t random_word(){uint32_t seed=uint32_t(uint64_t(uint32_t(l(0xf722)))*0x278dde6du);sl(0xf722,d(seed));return uint16_t(seed>>16);}
inline uint16_t random_bound(unsigned bound){unsigned value=random_word();bound&=65535;return uint16_t(bound?value%bound:value);}
inline int32_t wa(int32_t v){return w(w(v)*s(0x1e5e)/s(0x1b7f));}
inline int32_t uwa(int32_t v){return w(w(v)*s(0x1b7f)/s(0x1e5e));}
inline int32_t dwa(int32_t v){return rdiv(mul(v,s(0x1e5e)),s(0x1b7f));}
}
