#include "audio.hpp"
#include "contacts.hpp"
#include "state.hpp"
#include "runtime.hpp"
using namespace cc;
using namespace cc::state;
extern "C" {
uint32_t cc_aircraft_damage(uint32_t);
void cc_aircraft_home();
uint32_t cc_combat_hit(uint32_t,uint32_t);
uint32_t cc_combat_slam(uint32_t,uint32_t);
uint32_t cc_collision_fast(uint32_t,uint32_t);
uint32_t cc_collision_box(uint32_t,uint32_t);
uint32_t cc_collision_near(uint32_t,uint32_t);
}
namespace {
bool valid(unsigned p){return p<=65536-74;}
uint32_t magnitude32(int32_t v){return v<0?0u-uint32_t(v):uint32_t(v);}
unsigned relative(unsigned p,unsigned target,bool reduce){
    unsigned mask=0;
    for(unsigned axis=0;axis<3;++axis){
        int32_t delta=d(int64_t(l(p+4*axis))-l(target+4*axis));
        sl(0xe7e + 4*axis,delta);mask|=(magnitude32(delta)*2u)>>16;
    }
    // rel3d's optional map-distance reduction. relobj5d bypasses this.
    unsigned z0=u(0x1efa);int32_t above=w(-s(0xe88));
    if(reduce&&z0&&above>=0&&unsigned(above)>z0){
        unsigned denominator=(magnitude32(l(0xe86))*16u)>>16;
        unsigned numerator=uint16_t(z0*16u);
        if(denominator>numerator){
            unsigned factor=((uint32_t(numerator)<<16)/denominator)>>1;
            sw(0x1efc,factor);mask=0;
            for(unsigned axis=0;axis<3;++axis){
                int64_t product=int64_t(l(0xe7e + 4*axis))*factor;
                int64_t shifted=product>=0?(product/65536)*2:-((-product+65535)/65536)*2;
                sl(0xe7e + 4*axis,d(shifted));
                mask|=uint16_t(absw(w(uint64_t(product)>>32)));
            }
        }
    }
    return mask;
}
bool inside(unsigned p){
    for(unsigned axis=0;axis<3;++axis)
        if(magnitude32(l(0xe7e + 4*axis))>=u(p+66+2*axis))return false;
    return true;
}
void save_type(unsigned p){sw(0x1e64,u(p+24));sw(0x1e66,u(uint16_t(0x2d44+2*u(p+72))));}
void boom_sound_state(){
    unsigned hold=u(0x1da);
    if(hold==65535||(hold&&uint16_t(raw_ticks()-hold)<512))return;
    if(hold){cc_audio_preset(2);sw(0x1da,0);}
    uint16_t elapsed=uint16_t(raw_ticks()-u(0xf15f));if(elapsed>=512){sw(0xf15f,raw_ticks());cc_audio_explosion(elapsed);}
}
}
// F3DVEC.ASM qwalk, original 3.42 CS:7765. The caller gates ejection.
extern "C" uint32_t cc_contact_walk(uint32_t p){
    if(!valid(p))return 2;
    save_type(p);
    if(b(0x205d))return 0;
    if(relative(p,0xb05,true)&0xff00)return 0;
    if(!(u(0x1e66)&(64|128|512))||!inside(p))return 0;
    if(u(0x1e66)&128){
        if(b(0xae6))return 0;
        sw(0x1e6,u(p+72));wb(0xe58,10);sw(0x1e4,u(0x1e4)|0x800);
        for(unsigned i=0;i<12;++i)wb(0x1f06+i,b(0xe7e + i));
        return 0;
    }
    if(u(0x1e66)&512){
        if(!b(0xae6)){sw(0x1e6,u(p+72));wb(0xaf2,255);}
        return 0;
    }
    int32_t surface=l(p+8);
    if(u(0x1e66)&256)surface=d(int64_t(surface)+u(p+70)-721);
    if(d(int64_t(l(0xb0d))+720-surface)>=0)sl(0x2bf,surface);
    return 0;
}
// F3DVEC.ASM qcrash, original 3.42 CS:7860. Aboard, dxvec/rdf are
// deliberately the post-render scratch values; recomputing them changes play.
extern "C" uint32_t cc_contact_crash(uint32_t p){
    if(!valid(p))return 2;
    sw(0x1e64,u(p+24));if(u(p+72)==28)return 0;
    sw(0x1e66,u(uint16_t(0x2d44+2*u(p+72))));
    if(s(p+66)<0){
        source_error(b(p+66)==254?98:97);
        if(u(p+72)!=31)source_error(59);
        sw(0x1f00,u(p+72));wb(0xafc,255);
    }
    if(b(0xafc)){
        if(!u(0x5bc8)||relative(p,0x5bb0,false)&0xfff0)return 0;
    }else if(b(0x205d))return 0;
    if(!(u(0x1e64)&0x20)&&!(u(0x1e66)&16))return 0;
    if(!inside(p))return 0;
    if(u(0x1e66)&16){wb(0xf53,1);sl(0x2bb,l(p+8));return 0;}
    if((u(0x1e66)&0x20)||(u(0x1e64)&8)){
        if(u(0x1e66)&4){unsigned status=cc_combat_hit(p,65535);if(status)return status;}
        else sw(p+52,0);
    }
    unsigned damage=1,flags=u(0x1e64);bool skip_heavy=false;
    if(flags&0x40){
        damage=6;sw(0xe43,8);
        skip_heavy=b(0x2158)==4;
        if(!skip_heavy)wb(0x2158,7);
    }
    if(!skip_heavy&&(flags&0x80)){
        damage=75;if(u(0x1c7)>75)return 0;
        wb(0x2158,4);sw(0xe43,0);
        sw(0x1dc2,sar(w(-s(0x1dc2)),1));sw(0x1dc4,sar(w(-s(0x1dc4)),1));
        sw(0x1e4,u(0x1e4)|0x80);sw(0x1fc5,u(p+72));
    }
    if(u(0x1e66)&8){
        if(uint16_t(raw_ticks()-u(0x1ef4))<200)return 0;
        sw(0x1ef4,raw_ticks());damage*=2;
    }
    // OPL writes are handled by browser audio; retain their shared timer state.
    if(b(0xf54)||!b(0xafc)){cc_audio_collision_blam();sw(0x1da,65535);}
    else{sw(0x2c20,u(p+72));boom_sound_state();}
    return cc_aircraft_damage(damage);
}
// F3DVEC.ASM qcollis, original 3.42 CS:7413. Fast objects scan backwards;
// other objects test only the next entry. Callback mutation is visible later.
extern "C" uint32_t cc_contact_pairs(uint32_t source,uint32_t slot){
    if(!valid(source)||slot>65534)return 2;
    unsigned table=u(0x2955);
    if(table>65532)return 2;
    unsigned count=u(table),first=table+4,end=first+2*count;
    if(end>65536||slot<first||slot>=end||((slot-first)&1))return 2;
    unsigned type=u(source+72);
    if(type==21||type==30){
        if(!u(source+24)||!u(source+52))return 0;
        unsigned back=uint16_t(slot-2);
        while(w(back-uint16_t(first))>=0){
            unsigned target=u(back);if(!valid(target))return 2;
            if(b(source+19)!=b(target+19)&&u(target+24)){
                unsigned result=((u(source+24)|u(target+24))&0x100)
                    ?cc_collision_box(source,target):cc_collision_fast(source,target);
                if(result==1)return cc_combat_slam(source,target);
                if(result==2)return 0;
                if(result==3)return 2;
            }
            back=uint16_t(back-2);
        }
        return 0;
    }
    if(slot+2>=end)return 2;
    unsigned target=u(slot+2);if(!valid(target))return 2;
    if(b(source+19)==b(target+19))return 0;
    unsigned result=cc_collision_near(source,target);
    if(result==1)return cc_combat_slam(source,target);
    return result==3?2:0;
}
extern "C" uint32_t cc_contact_home(){cc_aircraft_home();return 0;}
