#include "demo_frame.hpp"
#include "edition.hpp"
#include "audio.hpp"
#include "state.hpp"
using namespace cc;using namespace cc::state;
extern "C" uint32_t cc_bullet_spawn();
extern "C" uint32_t cc_missile_spawn();
// Ordinary keyboard path. Mask:1 Space,2 left Shift,4 C. Rate counters use
// the original clock adapter, not the rendered-frame count.
extern "C" void cc_weapon_configure(uint32_t keys){
    bool slow=b(0xafc)||(keys&2);sw(0x1e3c,slow?700:4400);sw(0x1ca1,slow?1:2);sw(0x1e3e,slow?48:96);
}
extern "C" uint32_t cc_weapon_input(uint32_t keys){
    if(cc_demo_frame_mode()==255)return cc_demo_weapon_playback();
    if(b(0xaf6))return 1;
    if(!s(0x1e5e)||!s(0x1b7f))return 2;
    if(!u(0x1c9f)){
        sw(0x1c9f,u(0x1ca1));for(unsigned p=0x1ca3;p<=0x1ca5;p+=2)if(u(p))sw(p,u(p)-1);
        bool shift=u(0xe95)||(u(0xe93)>=1440&&(!b(0xf55)||u(0xe93)>10800));
        if(((keys&1)||(shift&&(keys&2)))&&u(0x1ca3)<=32){wb(0xf58,0);cc_bullet_spawn();}
    }
    if(!u(0x1ca7)){
        sw(0x1ca7,u(0x1ca9));if(keys&4)cc_demo_missile_request();
        if((keys&4)&&u(0x1ca5)<=16&&!b(0xf7a)){
            wb(0xf58,0);if(!b(0xafc)||b(0xf54)){cc_audio_missile(b(0xf5b2)!=0);if(cc_missile_spawn())sw(0x1ca5,u(0x1ca5)+5);}
        }
    }
    return 0;
}
