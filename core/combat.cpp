#include "state.hpp"
using namespace cc;using namespace cc::state;
extern "C" {
uint32_t cc_effect_boom(uint32_t);uint32_t cc_effect_flotsam(uint32_t,uint32_t);uint32_t cc_effect_shards(uint32_t,uint32_t);
uint32_t cc_enemy_hit(uint32_t,uint32_t);
}
namespace {
void increment(unsigned p){sw(p,u(p)+1);}
void pieces(unsigned p,unsigned count,bool shards){wb(0x1cd,0);if(shards)cc_effect_shards(p,count);else cc_effect_flotsam(p,count);}
unsigned damage(unsigned other){unsigned flags=b(other+24);return flags&0x80?25:flags&0x40?6:1;}
bool destroy_after_hit(unsigned object,unsigned other){unsigned hp=uint8_t(b(object+27)-damage(other));if(!hp||(hp&128))return true;wb(object+27,hp);return false;}
void missile_hit(unsigned object){wb(0xf15e,0x20);cc_effect_boom(object);wb(0x1cd,1);cc_effect_flotsam(object,3);sw(object+24,0);}
}
// Original direct hit paths delegate the remaining template callbacks to the
// source-derived enemy dispatcher; its status propagates to the caller.
extern "C" uint32_t cc_combat_hit(uint32_t object,uint32_t other){
    if(object>65536-74||(other!=65535&&other>65536-74))return 2;
    switch(u(object+72)){
    case 0:{
        unsigned quality=(u(object+24)>>12)&7;
        if(other!=65535){
            if(quality==4)return 0;
            if(!destroy_after_hit(object,other)){if(damage(other)>=6)pieces(object,2,true);return 0;}
        }else if(quality==4&&!b(0xafc)){wb(0xe58,11);sw(0x1e4,u(0x1e4)|0x1000);}
        wb(object+27,0);for(unsigned a=40;a<=44;a+=2)sw(object+a,0x3227);
        sw(object+24,(u(object+24)&0xff00)|0x11);sw(object+72,43);increment(0x1b81);
        pieces(object,12,false);pieces(object,10,true);increment(0x256+quality*2);return 0;
    }
    case 27:
        if(!destroy_after_hit(object,other))return 0;
        wb(object+27,0);sw(object+40,0x3521);sw(object+42,0x3569);sw(object+44,0x3569);sw(object+24,0x11);sw(object+72,33);increment(0x1ea);pieces(object,12,false);return 0;
    case 30:
        // Original bdest's legacy JS sees the incoming source type-index
        // arithmetic flags; normal slam dispatch enters with sign clear.
        wb(0xec36,b(other+19)&3);wb(0x1cd,1);cc_effect_flotsam(object,2);wb(0xec36,0);sw(object+24,0);return 0;
    case 21:missile_hit(object);return 0;
    case 25:case 26:{
        int value=w(s(0xf0da)-1);sw(0xf0da,value<0?0:value);
        if(b(object+19)==3){wb(0xf15e,0x20);cc_effect_boom(object);sw(object+24,0);}return 0;
    }
    case 8:case 9:sw(object+24,0);return 0;
    default:return cc_enemy_hit(object,other);
    }
}
extern "C" uint32_t cc_combat_slam(uint32_t source,uint32_t target){
    if(source>65536-74||target>65536-74)return 2;
    if(u(source+24)&8){unsigned status=cc_combat_hit(source,target);if(status)return status;}
    if(u(target+24)&8)return cc_combat_hit(target,source);
    return 0;
}
extern "C" void cc_complete_objective(uint32_t object){
    if(object>65536-74||(u(object+24)&0x200))return;
    increment(0x22c);sw(object+24,u(object+24)|0x200);if(!b(0xf3c))wb(0xf3c,255);
    sw(0xf7f,u(0x1c7)>=16?0x1127:0x102f);
}
