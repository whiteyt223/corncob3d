#include "demo_frame.hpp"
#include "edition.hpp"
#include "late_weapons.hpp"
#include "state.hpp"
#include "runtime.hpp"
#include "audio.hpp"
#include "hud.hpp"
#include "flight_lifecycle.hpp"
using namespace cc;using namespace cc::state;
extern "C" {
uint32_t cc_new_effect(uint32_t);
void cc_matvmul(const uint16_t*,const uint16_t*,uint16_t*);
uint8_t* cc_video_memory();
}
namespace {
uint8_t cockpit_image[640*350];bool cockpit_ready=false;uint32_t restore_sequence=0;
void copy(unsigned from,unsigned to,unsigned count){for(unsigned i=0;i<count;++i)wb(to+i,b(from+i));}
void subtract_pair(unsigned p,int value){unsigned old=u(p),sub=uint16_t(value);sw(p,old-sub);sw(p+2,u(p+2)-(old<sub));}
bool alt(){return (b(0x2ce)&1)!=0;}
}
extern "C" uint8_t* cc_weapon_cockpit_image(){return cockpit_image;}
extern "C" void cc_weapon_cockpit_ready(uint32_t value){cockpit_ready=value!=0;}
extern "C" uint32_t cc_weapon_restore_sequence(){return restore_sequence;}
extern "C" void cc_weapon_interior_view(){
    if(!cc_runtime_state()[1]){
        sw(0x1c3e,0x1bf6);sw(0x1c40,0x1c0e);
        if(cockpit_ready){auto* video=cc_video_memory();for(unsigned i=0;i<sizeof(cockpit_image);++i){video[i]=cockpit_image[i];video[0x7e00*8+i]=cockpit_image[i];}}
        ++restore_sequence;wb(0xe57,255);
    }
}
extern "C" void cc_weapon_restore_view(){
    if(u(0x1c3e)==0x1c26)return;
    if(u(0x1c3e)!=0x1bf6){source_error(29);return;}
    // The HUD calls still run when interiorview is gated by source_error.
    cc_weapon_interior_view();cc_hud_damage_init();cc_hud_redraw_buttons();
}
extern "C" uint32_t cc_weapon_bomb_from(uint32_t source){
    // This accepts original observer0B05 too, whose trailing fields deliberately
    // come from adjacent packed DS storage, just as strtbomb1 reads them.
    if(source>65536-74)return 2;
    unsigned p=cc_new_effect(20);if(!p)return 1;
    wb(p+18,b(source+18));wb(p+19,2);sw(p+52,3200);sw(p+24,u(p+24)|0x8000);
    copy(source,p,12);copy(source+46,p+46,6);return 0;
}
extern "C" uint32_t cc_weapon_drop_bomb(){
    unsigned p=cc_new_effect(20);if(!p)return 1;
    wb(p+18,0);sw(p+52,3200);sw(p+24,(u(p+24)|0x8000)&0xff17);
    copy(0xe8b,p,18);
    // Literal original overlapping SUB/SBB chain, not a vector recoil model.
    int velocity=sar(s(0x1dc2),4);subtract_pair(0x1dc2,velocity);sw(p+46,velocity);
    velocity=sar(s(0x1dc4),4);sw(p+48,velocity);subtract_pair(0x1dc6,velocity);
    velocity=sar(s(0x1dc6),4);sw(p+50,velocity);subtract_pair(0x1dca,velocity);
    sw(0x1d06,-500);sw(0x1d08,0);sw(0x1d0a,-500);
    uint16_t matrix[9],vector[3],result[3];for(unsigned i=0;i<9;++i)matrix[i]=u(0x1d48+i*2);for(unsigned i=0;i<3;++i)vector[i]=u(0x1d06+i*2);
    cc_matvmul(matrix,vector,result);for(unsigned i=0;i<3;++i){sw(0x1d0c+i*2,result[i]);addl(p+i*4,w(result[i]));}
    return 0;
}
extern "C" uint32_t cc_weapon_late_input(uint32_t ticks){
    const unsigned now=uint16_t(ticks);
    if(b(0x26c)==0x9f){
        wb(0x26c,0);unsigned value=b(0x1e3b)^255;
        if(!value){cc_audio_sound_off();wb(0x1e3b,b(0x1e3b)^255);}
        else{wb(0x1e3b,value);cc_audio_sound_on(b(0xafc)!=0);wb(0x19ba,0);wb(0x19bc,0);sw(0x1d6,64);}
    }
    if(b(0x26c)==0x8a)wb(0xae7,255);
    wb(0xf1fc,1+b(0x19f3));
    unsigned elapsed=uint16_t(now-u(0x19f6)),denominator=u(0x1e60);uint32_t product=elapsed*u(0x1e5e);
    sw(0x19f6,now);
    if((product>>16)<denominator){
        unsigned total=uint16_t(product/denominator+u(0x19f4));
        if(total<=2184)sw(0x19f4,total);
        else{wb(0xf1fc,10);if(!alt()&&(!b(0xafc)||b(0xf54))&&b(0x26c)==0x97)wb(0x19f3,11);}
    }
    if(b(0xe4e)<3){
        elapsed=uint16_t(now-u(0xe4c));product=elapsed*u(0x1e5e);unsigned value=elapsed;
        if((product>>16)<u(0x1e5e)){
            // Source performs DIV without a second guard; report that exact
            // invalid arithmetic domain instead of causing host UB.
            if(!denominator||(product>>16)>=denominator)return 2;
            value=product/denominator;
        }
        if(value>=u(0xe4f)){wb(0xe4e,b(0xe4e)+1);cc_hud_button(b(0xe4e)+5,0x0807);sw(0xe4c,now);}
    }
    if(cc_edition_is_other_worlds()?cc_demo_bomb_input():b(0x26c)==0xb0){
        if(!cc_edition_is_other_worlds())wb(0x26c,0);
        if(b(0xe4e)&&(!b(0xafc)||b(0xf54))){cc_weapon_drop_bomb();wb(0xe4e,b(0xe4e)-1);cc_hud_button(b(0xe4e)+6,0x0708);sw(0xe4c,now);}
    }
    if(b(0x26c)!=0xa7){
        if(!alt())return 0;
        if(b(0x26c)==0x97){
            wb(0x26c,0);wb(0xae6,cc_edition_is_other_worlds()?b(0xae6)^255:255);
            if(b(0xae6))cc_aircraft_clear_damage();
            cc_weapon_restore_view();
        }
        if(b(0x26c)!=0x99)return 0;
    }
    wb(0x26c,0);
    if(!b(0xafc)||!b(0x1d03))return 0;
    cc_weapon_bomb_from(0xb05);
    // strtbomb1 carry is intentionally ignored. Full-pool failure leaves
    // expobj pointing at the prototype, which this source path then mutates.
    unsigned p=u(0xec28);sw(p+46,0);sw(p+48,0);sw(p+50,0);sw(p+66,0);sw(p+14,-16384);sw(p+24,(u(p+24)&~8u)|0x804);wb(p+19,0);wb(0x1d03,b(0x1d03)-1);
    return 0;
}
