#include "audio.hpp"
#include "state.hpp"
#include "runtime.hpp"
using namespace cc;using namespace cc::state;
extern "C" uint32_t cc_new_effect(uint32_t);
extern "C" uint32_t cc_projectile_move(uint32_t,uint32_t);
extern "C" uint32_t cc_table_add(uint32_t,uint32_t);
namespace {
bool valid(unsigned p){return p<=65536-74;}
void copy(unsigned from,unsigned to,unsigned size){for(unsigned i=0;i<size;++i)wb(to+i,b(from+i));}
void add_velocity(unsigned p,unsigned axis,int value){sw(p+46+axis*2,s(p+46+axis*2)+value);}
void queue(unsigned p,unsigned life){wb(p+18,0);sw(p+52,life);sw(p+24,u(p+24)|0x8000);}
void small_velocity(unsigned p){add_velocity(p,0,int(random_bound(100))-50);add_velocity(p,1,int(random_bound(100))-50);add_velocity(p,2,random_bound(450));}
}
// Return original carry: 0 completed (including a source cap), 1 allocation
// failed. 2 denotes an invalid host input. Sound is dispatched separately.
extern "C" uint32_t cc_effect_puff(uint32_t source){
    if(!valid(source))return 2;
    unsigned count=random_bound(4)+1;
    for(unsigned i=0;i<count;++i){unsigned p=cc_new_effect(18);if(!p)return 1;copy(source,p,8);sl(p+8,0);small_velocity(p);queue(p,7);}
    return 0;
}
extern "C" uint32_t cc_effect_boom(uint32_t source){
    if(!valid(source))return 2;
    unsigned p=cc_new_effect(19);if(!p)return 1;copy(source,p,12);small_velocity(p);queue(p,5);wb(p+24,b(p+24)|b(0xf15e));
    unsigned hold=u(0x1da);
    if(hold==65535||(hold&&uint16_t(raw_ticks()-hold)<512))return 0;
    if(hold){cc_audio_preset(2);sw(0x1da,0);}
    uint16_t elapsed=uint16_t(raw_ticks()-u(0xf15f));if(elapsed>=512){sw(0xf15f,raw_ticks());cc_audio_explosion(elapsed);}
    return 0;
}
extern "C" uint32_t cc_effect_flotsam(uint32_t source,uint32_t requested){
    if(!valid(source)||!s(0x1e5e)||!s(0x1b7f))return 2;
    unsigned remaining=uint16_t(requested);
    if(b(0xec36)!=3){unsigned half=remaining>>1;remaining=uint16_t(random_bound(half)+half+1);}
    do{
        if(uint16_t(u(0xf0da)+remaining)>=u(0xf0dc)){if(remaining>=26)source_error(163);return 0;}
        sw(0x8686+48,random_bound(4000));unsigned type=26;
        if(b(0xec36)!=3&&w(random_word())<0)type=25;
        unsigned p=cc_new_effect(type);if(!p)return 1;
        copy(source,p,12);sw(p+8,u(p+8)+750);copy(source+46,p+46,6);
        for(unsigned a=0;a<2;++a)add_velocity(p,a,int(random_bound(u(0xec2a)))-sar(s(0xec2a),1));
        int dz=b(0x1cd)?int(random_bound(u(0xec2a)))-sar(s(0xec2a),1):random_bound(u(0xf158));add_velocity(p,2,dz);
        int bound=uwa(b(0x1cd)?18:180);queue(p,uint16_t(random_bound(uint16_t(bound))+sar(bound,2)));wb(p+19,b(0xec36));
        if(b(0xec36)==3){sw(p+24,u(p+24)|0x30);wb(p+65,1);}
        remaining=uint16_t(remaining-1);
    }while(remaining);
    return 0;
}
extern "C" uint32_t cc_effect_shards(uint32_t source,uint32_t requested){
    if(!valid(source))return 2;
    unsigned half=uint16_t(requested)>>1,remaining=uint16_t(random_bound(half)+half+1);
    do{
        unsigned type=w(random_word())<0?9:8,p=cc_new_effect(type);if(!p)return 1;
        copy(source,p,12);sw(p+8,u(p+8)+750);copy(source+46,p+46,6);
        for(unsigned a=0;a<2;++a){int bound=sar(s(0xec2a),2);add_velocity(p,a,int(random_bound(uint16_t(bound)))-sar(bound,1));}
        int dz=b(0x1cd)?int(random_bound(u(0xec2a)))-sar(s(0xec2a),1):random_bound(1500);add_velocity(p,2,dz);
        queue(p,6200);for(unsigned a=0;a<3;++a)sw(p+54+a*2,random_bound(4000));
        remaining=uint16_t(remaining-1);
    }while(remaining);
    return 0;
}
extern "C" uint32_t cc_move_bullets(){
    sw(0x1ce,u(0x1e40));sw(0x1d2,54);
    for(unsigned i=0;i<32;++i){unsigned p=0x7732+i*74;if(s(p+52)<=0){sw(p+24,0);continue;}
        sw(p+52,u(p+52)-1);unsigned status=cc_projectile_move(p,1);if(status==2)return 2;if(status&1)cc_effect_puff(p);
    }return 0;
}
extern "C" uint32_t cc_move_effects(){
    for(unsigned p=0x8fa6;p<0xec26;p+=74){
        if(u(p+24)&0x8000){sw(p+24,u(p+24)&0x7fff);if(cc_table_add(u(0x2393),p))return 2;}
        if(u(0x1efa)||!u(p+24))continue;
        if(s(p+52)<=0){if(b(p+65)){wb(0xf15e,0x20);cc_effect_boom(p);}sw(p+24,0);continue;}
        sw(p+52,u(p+52)-1);sw(0x1ce,u(p+60));sw(0x1d0,u(p+62));sw(0x1d2,b(p+64));
        if(cc_projectile_move(p,0)==2)return 2;
    }return 0;
}
