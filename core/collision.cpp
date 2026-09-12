#include "state.hpp"
using namespace cc;using namespace cc::state;
extern "C" uint32_t cc_vmag(int32_t,int32_t,int32_t);
namespace {
bool valid(unsigned p){return p<=65536-74;}
uint32_t unsigned_abs(int32_t value){return value<0?0u-uint32_t(value):uint32_t(value);}
int get_delta(unsigned source,unsigned target){
    for(unsigned a=0;a<3;++a){int32_t delta=d(int64_t(l(source+a*4))-l(target+a*4));sw(0x2026+a*2,delta);if(unsigned_abs(delta)>=32768)return -1;}
    uint32_t distance=cc_vmag(s(0x2026),s(0x2028),s(0x202a));if(distance>65535)return -2;sw(0x2032,distance);return int(distance);
}
uint64_t quotient(uint64_t numerator,int32_t divisor){
    bool negative=(numerator>>63)!=0;
    if(!divisor)return negative?1:~uint64_t(0);
    uint64_t magnitude=negative?0-numerator:numerator;
    uint32_t denominator=divisor<0?0u-uint32_t(divisor):uint32_t(divisor);
    uint64_t value=magnitude/denominator;
    return negative!=(divisor<0)?0-value:value;
}
}
// Results: 0 skipped, 1 collision, 2 aligned but too far (stop backscan),
// 3 invalid arithmetic/input. Callback execution belongs to the caller.
extern "C" uint32_t cc_collision_fast(uint32_t source,uint32_t target){
    if(!valid(source)||!valid(target))return 3;
    if(!(u(target+24)&8))return 0;
    int distance=get_delta(source,target);if(distance<0)return distance==-1?0:3;
    unsigned radius=u(target+66)>u(source+66)?u(target+66):u(source+66);if(unsigned(distance)<=radius)return 1;
    uint32_t speed=cc_vmag(s(source+46),s(source+48),s(source+50));if(speed>65535)return 3;sw(0x2034,speed);
    int32_t dot=0;
    for(unsigned a=0;a<3;++a){int velocity=s(source+46+a*2);sw(0x202c+a*2,velocity);dot=d(int64_t(dot)+s(0x2026+a*2)*velocity);}
    sl(0x203a,dot);sl(0x203e,dot);
    uint64_t q=quotient((uint64_t(uint32_t(dot))<<32)|uint32_t(l(0x2036)),d(uint32_t(distance)*speed));
    sl(0x2036,d(uint32_t(q)));sl(0x203a,d(uint32_t(q>>32)));
    if(!(q>>63))return 0;
    if(((q>>32)&65535)==65535&&((0u-uint32_t(q))>>16)<45000)return 0;
    return (unsigned(distance)>>1)<=speed?1:2;
}
extern "C" uint32_t cc_collision_box(uint32_t source,uint32_t target){
    if(!valid(source)||!valid(target))return 3;
    if(!u(source+66)||!u(target+66))return 0;
    for(unsigned a=0;a<3;++a)if(unsigned_abs(d(int64_t(l(source+a*4))-l(target+a*4)))>=unsigned(u(source+66+a*2))+u(target+66+a*2))return 0;
    return 1;
}
extern "C" uint32_t cc_collision_near(uint32_t source,uint32_t target){
    if(!valid(source)||!valid(target))return 3;
    unsigned flags=u(source+24)|u(target+24);if(!(flags&8))return 0;
    if(flags&0x100)return cc_collision_box(source,target);
    if(!u(source+66)||!u(target+66))return 0;
    unsigned radius=u(target+66);if(radius<u(source+66))radius=uint16_t(radius+u(source+66));
    int distance=get_delta(source,target);if(distance<0)return distance==-1?0:3;
    return unsigned(distance)<radius?1:0;
}
