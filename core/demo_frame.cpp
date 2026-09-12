#include "demo_frame.hpp"
#include "demo.hpp"
#include "edition.hpp"
#include "state.hpp"
#include "runtime.hpp"
#include "audio.hpp"
#include "flight_lifecycle.hpp"
using namespace cc;using namespace cc::state;
extern "C" uint32_t cc_bullet_spawn();
extern "C" uint32_t cc_missile_spawn();
namespace {
const uint8_t* file=nullptr;uint32_t io[6]={};uint8_t output[74];
unsigned input_ax=0;
uint8_t* demo_state(){return cc_flight_state()+0xe8a;}
void stop_read(){wb(0xeaf,0);wb(0x19ec,255);io[1]|=8;}
bool record_read(bool raw){
    if(io[2]>=io[3]){io[5]=1;return false;}
    const uint8_t* input=file+io[2];
    if(input[0]==255){++io[2];if(!raw)wb(0xe8a,255);io[1]|=16;stop_read();return true;}
    if(raw){
        const unsigned count=(input[0]&128)?8:37;
        if(count>io[3]-io[2]){io[5]=1;return false;}
        wb(0xe8a,input[0]);io[2]+=count;return true;
    }
    const unsigned result=cc_demo_decode(demo_state(),input,io[3]-io[2],b(0xafc));
    if(result==UINT32_MAX){io[5]=1;return false;}
    io[2]+=result>>16;if(uint8_t(result))source_error(result);
    if(cc_demo_apply(demo_state(),cc_flight_state()+0xb05,cc_flight_state()+0x1dc2,u(0x1b7f),b(0xafc))){io[5]=2;return false;}
    return true;
}
void record(){
    cc_demo_update(demo_state(),cc_flight_state()+0xb05,u(0x1bc8));
    const unsigned result=cc_demo_record(demo_state(),cc_flight_state()+0xb05,cc_flight_state()+0x26c,output);
    io[0]=result&65535;io[1]|=result>>16;if(io[0])++io[4];
}
}
extern "C" void cc_demo_frame_reset(const uint8_t* bytes,uint32_t size,uint32_t request){
    file=bytes;for(unsigned& value:io)value=0;io[3]=size;input_ax=0;
    if(cc_edition_is_other_worlds())wb(0xaeb,request?255:0);
}
extern "C" void cc_demo_frame_begin(){io[0]=0;io[1]=0;io[5]=0;}
extern "C" uint32_t cc_demo_frame_mode(){return cc_edition_is_other_worlds()?b(0xeaf):0;}
extern "C" void cc_demo_frame_timing(){if(cc_edition_is_other_worlds())sw(0x1e5e,cc_demo_ticks(demo_state(),u(0x1e5e)));}
extern "C" uint32_t* cc_demo_frame_io(){return io;}
extern "C" uint8_t* cc_demo_frame_output(){return output;}
extern "C" uint32_t cc_demo_frame_before_refresh(){
    if(!cc_edition_is_other_worlds()||b(0xafc))return 0;
    if(u(0x1bc8)>=6&&b(0xaeb)){
        wb(0xf7b,0);wb(0xaeb,0);
        if(b(0xeaf)){stop_read();return 0;}
        if(u(0x1e5e)>18)wb(0xebe,255);
        io[1]|=4;io[2]=0;
        if(!file){source_error(49);wb(0xeaf,0);wb(0xe8a,255);for(unsigned i=0;i<37;++i)output[i]=demo_state()[i];io[0]=37;io[1]|=2;++io[4];return 0;}
        wb(0xeaf,255);sw(0x1de2,u(0x1de6)-1);
        if(!record_read(false))return 2;
        if(b(0xe8a)==255)return 0;
        for(unsigned i=0;i<12;++i)wb(0x285+i,b(0xb05+i));
        wb(0x8f88,1);wb(0xf77,0);return 0;
    }
    if(b(0xeaf)!=255)return 0;
    if(!record_read(false))return 2;
    if(b(0xe8a)==255||!b(0xebe))return 0;
    const unsigned previous=b(0xe8a);
    if(!record_read(false))return 2;
    if(b(0xe8a)!=255)cc_demo_merge(demo_state(),previous);
    return 0;
}
extern "C" uint32_t cc_demo_frame_record_tail(){
    if(!cc_edition_is_other_worlds())return 0;
    if(b(0xafc)&&b(0xeaf)==255){
        if(!record_read(true))return 2;
        // Ejected EOF jumps into the normal observer-matrix/update tail.
        return (io[1]&16)?3:0;
    }
    record();return 0;
}
extern "C" uint32_t cc_demo_frame_controls(uint32_t plus,uint32_t forward,uint32_t backward){
    if(!cc_edition_is_other_worlds())return 0;
    int ax=clamp(w(sar(absw(w(sar(s(0x1d9c),4)-231)),4)+sar(s(0x1de2),2)),128,768);
    if((!b(0xafc)||b(0xf54))&&plus&&(!(u(0x1de)&0x2000)||b(0xaec))&&u(0x1de6)){
        ax=s(0x1de6);if(w(ax-w(s(0x1de2)+20))<0)ax=w(ax-3);
    }
    // Invincible crash returns through resetplane's XOR AX,AX.
    if(b(0xe58)&&b(0xae6))ax=0;
    if(b(0xafc)&&(forward||backward)){
        const int elapsed=absw(w(raw_ticks()-u(0xe4a)));
        ax=elapsed<=40?elapsed:0;
    }
    input_ax=uint16_t(ax)&0xff00;return input_ax;
}
extern "C" uint32_t cc_demo_flap_input(){
    if(!cc_edition_is_other_worlds())return 0;
    int ax;
    if(b(0xeaf)==255){wb(0xf59,b(0xe8a)&16);ax=w(input_ax|b(0xf59));}
    else{if(b(0x26c)!=0xa1)return 0;wb(0x26c,0);wb(0xf59,b(0xf59)^255);ax=200;}
    wb(0xe8a,(b(0xe8a)&239)|(b(0xf59)&16));
    if(!b(0xf77))sw(0x1dfc,s(0x1dfc)+(b(0xf59)?-ax:ax));
    return 1;
}
extern "C" uint32_t cc_demo_bomb_input(){
    if(!cc_edition_is_other_worlds())return 0;
    if(b(0xeaf)==1)wb(0xe8a,b(0xe8a)&251);
    if(b(0xeaf)==255&&(b(0xe8a)&4))return 1;
    if(b(0x26c)!=0xb0)return 0;
    wb(0x26c,0);wb(0xe8a,b(0xe8a)|4);return 1;
}
extern "C" void cc_demo_missile_request(){if(cc_edition_is_other_worlds())wb(0xe8a,b(0xe8a)&253);}
extern "C" uint32_t cc_demo_weapon_playback(){
    if(!cc_edition_is_other_worlds()||b(0xeaf)!=255)return 1;
    if(!u(0x1c9f)){
        sw(0x1c9f,u(0x1ca1));for(unsigned p=0x1ca3;p<=0x1ca5;p+=2)if(u(p))sw(p,u(p)-1);
        if(!(b(0xe8a)&1)&&u(0x1ca3)<=32){wb(0xf58,0);cc_bullet_spawn();}
    }
    // Playback jumps past MOV mistick,misrate, so it deliberately leaves zero.
    if(!u(0x1ca7)&&!(b(0xe8a)&2)){
        cc_demo_missile_request();
        if(u(0x1ca5)<=16&&!b(0xf7a)){
            wb(0xf58,0);if(!b(0xafc)||b(0xf54)){cc_audio_missile(b(0xf5b2)!=0);if(cc_missile_spawn())sw(0x1ca5,u(0x1ca5)+5);}
        }
    }
    return 0;
}
extern "C" void cc_demo_eject_input(){
    if(!cc_edition_is_other_worlds())return;
    const bool injected=b(0xeaf)==255&&(b(0xe8a)&32);unsigned scan=b(0x26c);
    if(!injected){wb(0xe8a,b(0xe8a)&223);if(scan!=0x92)return;wb(0x26c,0);scan=0;}
    if(b(0xafc))return;
    wb(0xe8a,b(0xe8a)|32);cc_aircraft_eject();if(injected)wb(0x26c,scan);
}
extern "C" void cc_demo_chute_input(){
    if(!cc_edition_is_other_worlds())return;
    const bool injected=b(0xeaf)&&(b(0xe8a)&32);unsigned scan=b(0x26c);
    if(!injected){if(scan!=0xb9)return;wb(0x26c,0);scan=0;}
    cc_aircraft_pull_chute();if(injected)wb(0x26c,scan);
}
