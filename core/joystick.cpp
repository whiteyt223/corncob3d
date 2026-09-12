#include "demo_frame.hpp"
#include "edition.hpp"
#include "joystick.hpp"
#include "state.hpp"
#include "audio.hpp"
#include "runtime.hpp"
using namespace cc;
using namespace cc::state;
extern "C" uint32_t cc_lifecycle_arrows();
extern "C" uint32_t cc_bullet_spawn();
extern "C" uint32_t cc_missile_spawn();
namespace {
uint16_t host_x=513,host_y=513;
constexpr unsigned left=0xaf7,right=0xafa,center=0xafd,maximum=0xaff;
constexpr unsigned up=0xb01,down=0xb03,right_hash=0xb17,vcenter=0xb19,vmaximum=0xb1b;
unsigned actual_right(){return uint16_t(u(right)+u(right_hash))>>1;}
unsigned set_right(unsigned value){
    unsigned delta=uint16_t(u(right)-u(right_hash))>>1;
    sw(right,delta+value);sw(right_hash,value-delta);return u(right);
}
unsigned absolute_word(unsigned value){return uint16_t(absw(w(value)));}
uint32_t axis(unsigned raw,unsigned ctr,unsigned lo,unsigned hi){
    const bool positive=raw>ctr;
    uint32_t scaled=cc_joystick_scale(uint16_t(positive?raw-ctr:ctr-raw),uint16_t(positive?hi-ctr:ctr-lo));
    return scaled==UINT32_MAX?scaled:uint16_t(positive?512+scaled:512-scaled);
}
}
extern "C" uint32_t cc_joystick_scale(uint32_t displacement,uint32_t extent){
    unsigned a=uint16_t(displacement),c=uint16_t(extent);
    if(!c)return UINT32_MAX;
    if(w(a)<0)a=0;
    if(a>c)a=c;
    uint32_t product=a*0x205u;
    if((product>>16)>=c)product=((c-1)<<16)|(product&65535);
    a=product/c;product=a*a;
    if((product>>16)>=512)product=(511u<<16)|(product&65535);
    return product/512;
}
extern "C" uint32_t cc_joystick_select(uint32_t raw_x,uint32_t raw_y,uint32_t keyboard){
    if(!b(0xaf6)||keyboard){
        unsigned x=uint16_t(keyboard+512),y=uint16_t((keyboard>>16)+512);
        return x|(y<<16);
    }
    uint32_t x=axis(uint16_t(raw_x),u(center),u(left),actual_right());
    if(x==UINT32_MAX)return x;
    wb(0xb2b,0);
    if(absolute_word(x-u(center))<=u(0xb23))wb(0xb2b,b(0xb2b)-1);
    x=unsigned(clamp(w(x),0,1024));
    uint32_t y=axis(uint16_t(raw_y),u(vcenter),u(up),u(down));
    if(y==UINT32_MAX)return y;
    if(absolute_word(y-u(vcenter))<=u(0xb23))wb(0xb2b,b(0xb2b)-1);
    y=unsigned(clamp(w(y),0,1024));
    return x|(y<<16);
}
extern "C" uint32_t cc_joystick_input(uint32_t raw_x,uint32_t raw_y){
    host_x=uint16_t(raw_x);host_y=uint16_t(raw_y);
    return cc_joystick_select(raw_x,raw_y,cc_lifecycle_arrows());
}
extern "C" uint32_t cc_joystick_read_centered(){
    const uint32_t packed=cc_joystick_input(host_x,host_y);
    if(packed==UINT32_MAX)return packed;
    return uint16_t((packed&65535)-512)|(uint32_t(uint16_t((packed>>16)-512))<<16);
}
extern "C" uint32_t cc_joystick_brake(uint32_t keys){
    return ((keys&3)||(b(0xaf6)&&(keys&4)))?1:0;
}
extern "C" uint32_t cc_joystick_startup(uint32_t present,uint32_t raw_x,uint32_t raw_y){
    if(!b(0xaf6))return 0;
    if(!present){source_error(91);return 91;}
    if(b(0xaf5)&128)return 1;
    if(b(0xaf5)&1)return 0;
    unsigned distance=uint16_t(absolute_word(raw_x-u(center))+absolute_word(raw_y-u(vcenter)));
    return (uint16_t(distance-50)&32768)?0:1; // original JS, not signed JL
}
extern "C" void cc_joystick_calibration_begin(){sw(0xb1d,1);wb(0xae0,15);}
extern "C" uint32_t cc_joystick_calibration_sample(uint32_t stage,uint32_t raw_x,uint32_t raw_y){
    unsigned x=uint16_t(raw_x),y=uint16_t(raw_y);
    if(stage==0){sw(left,x);sw(up,y);return 1;}
    if(stage==1){
        unsigned hashed=set_right(x);sw(down,y);
        return w(hashed>>1)>s(left)&&w(y>>1)>s(up)?2:3;
    }
    if(stage!=2)return UINT32_MAX;
    sw(center,x);sw(vcenter,y);
    if(w(x>>1)<=s(left)||w(y>>1)<=s(up))return 3;
    unsigned total=uint16_t(u(right)+u(right_hash));
    if(w((total>>1)-(total>>5))<=s(center))return 3;
    if(w(u(up)-(u(up)>>3))>=s(vcenter))return 3;
    const int va=w(u(down)-u(vcenter)),vc=w(u(vcenter)-u(up));
    sw(vmaximum,va<=vc?va:vc);
    const int a=w(actual_right()-u(center)),c=w(u(center)-u(left));
    sw(maximum,a<=c?a:c);sw(0xb1d,0);wb(0xae0,0);return 0;
}
extern "C" uint32_t cc_joystick_calibration_abort(){source_error(93);wb(0xae0,0);return 93;}
extern "C" void cc_joystick_calibration_finish_reentry(){if(cc_runtime_state()[1]==93)source_error(0);wb(0x1baa,0);}
extern "C" uint32_t cc_joystick_weapons(uint32_t keys,uint32_t port){
    if(cc_demo_frame_mode()==255)return cc_demo_weapon_playback();
    if(!b(0xaf6))return 1;
    if(!s(0x1e5e)||!s(0x1b7f))return 2;
    if(!u(0x1c9f)){
        sw(0x1c9f,u(0x1ca1));
        for(unsigned p=0x1ca3;p<=0x1ca5;p+=2)if(u(p))sw(p,u(p)-1);
        if(b(0xeaf)==1)wb(0xe8a,(b(0xe8a)&252)|((port&48)>>4));
        if(!(port&16)&&u(0x1ca3)<=32){wb(0xf58,0);cc_bullet_spawn();}
    }
    if(!u(0x1ca7)){
        sw(0x1ca7,u(0x1ca9));if((keys&1)||!(port&32))cc_demo_missile_request();
        if(((keys&1)||!(port&32))&&u(0x1ca5)<=16&&!b(0xf7a)){
            wb(0xf58,0);
            if(!b(0xafc)||b(0xf54)){
                cc_audio_missile(b(0xf5b2)!=0);
                if(cc_missile_spawn())sw(0x1ca5,u(0x1ca5)+5);
            }
        }
    }
    return 0;
}
