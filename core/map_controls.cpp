#include "map_controls.hpp"
#include "state.hpp"
#include "runtime.hpp"
#include "audio.hpp"
#include "hud.hpp"
#include "late_weapons.hpp"
#include "universe_fields.hpp"
using namespace cc;using namespace cc::state;namespace f=universe_field;
extern "C" {
uint32_t cc_enemy_change_walls();
uint32_t cc_decode_object(uint32_t,int32_t,int32_t,uint32_t);
uint32_t cc_sin(uint32_t);int32_t cc_ssin(uint32_t);
void cc_calcmat(const uint16_t*,uint16_t*);void cc_ncalcmat(const uint16_t*,uint16_t*);
}
namespace {
bool active=false,return_cockpit=false,draw_pending=false;unsigned draw_tile=64,draw_index=0;
bool key(unsigned scan){return (b(0x2c7+scan/8)&(1u<<((-scan)&7)))!=0;}
void copy(unsigned from,unsigned to,unsigned count){for(unsigned i=0;i<count;++i)wb(to+i,b(from+i));}
void matrices(){
    uint16_t trig[6],matrix[9];
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<2;++j){unsigned angle=uint16_t(u(0xb11+i*2)+j*16384);wb(0xf378,cc_sin(angle)>>16);trig[i*2+j]=uint16_t(cc_ssin(angle));sw(0x20ae + i*4+j*2,trig[i*2+j]);}
    cc_calcmat(trig,matrix);for(unsigned i=0;i<9;++i)sw(0x1fde + i*2,matrix[i]);
    cc_ncalcmat(trig,matrix);for(unsigned i=0;i<9;++i)sw(0x1ff0+i*2,matrix[i]);
    for(unsigned i=0;i<3;++i)sw(0x20ae + i*4,-w(trig[i*2]));
}
bool adjusted(int v,int& out){if(!s(0x1b7f))return false;int q=w(v)*s(0x1e5e)/s(0x1b7f);if(q<-32768||q>32767)return false;out=w(q);return true;}
int32_t pan_amount(int input){return dwa(mul(d(uint32_t(l(0xb0d))>>4),input)/1024);}
void finish_outer(){
    if(return_cockpit){cc_audio_sound_on(b(0xafc)!=0);wb(0x19ba,0);wb(0x19bc,0);sw(0x1d6,64);wb(0xf50,255);cc_weapon_interior_view();cc_hud_damage_init();cc_hud_redraw_buttons();}
    return_cockpit=false;
}
}
extern "C" void cc_map_reset(){active=false;return_cockpit=false;draw_pending=false;draw_tile=64;draw_index=0;}
extern "C" uint32_t cc_map_active(){return active;}
extern "C" uint32_t cc_map_actor(){return b(0xafc)?0x80be:0x5bb0;}
extern "C" void cc_map_restore_observer(){copy(cc_map_actor(),0xb05,18);matrices();}
extern "C" uint32_t cc_map_begin(){
    wb(0xf529,b(0xf3b));wb(0xf3b,0);sw(0x6312,0);
    unsigned status=cc_enemy_change_walls();if(status)return status;
    wb(0x31b6,0);copy(0xb05,cc_map_actor(),18);copy(0xb05,0x5b66,18);
    sl(0xb0d,210<<16);sw(0x1efa,35);sw(0xb11,0);sw(0xb13,16384);sw(0xb15,0);matrices();active=true;return 0;
}
extern "C" uint32_t cc_map_request(){
    if(active)return 1;
    if(b(0x26c)==0xb2){wb(0x26c,0);wb(0xaef,255);if(b(0xaee)){sw(0xf7f,0x12ed);if(!b(0xaf1)){sw(0xf7f,0x129f);wb(0xaef,0);}}}
    if(!b(0xaef)||(b(0xaee)&&u(0xf7f)))return 0;
    wb(0xaef,0);return_cockpit=u(u(0x1c3e)+6)<=199;
    if(return_cockpit){
        if(u(0x1c3e)!=0x1c26){sw(0x1c40,0);wb(0xe57,0);sw(0x1c3e,0x1c26);}
        sw(0x1ef6,0x1fde);sw(0x1ef8,0x1ff0);cc_audio_sound_off();
    }
    unsigned status=cc_map_begin();return status?status:1;
}
extern "C" void cc_map_end(){wb(0x26c,0);sw(0x1efa,0);cc_map_restore_observer();wb(0xf3b,b(0xf529));active=false;}
extern "C" void cc_map_frame_begin(uint32_t ticks){
    wb(0x743b,(raw_ticks()&1024)?0:14);int dt=uint16_t(ticks)>>3;
    if(dt>=s(0xe51))dt=s(0xe51);
    if(dt<=s(0xe53))dt=s(0xe53);
    sw(0x1e5e,dt);sw(0xf3f,u(0xb13));
}
extern "C" uint32_t cc_map_bump_angles(int32_t pitch,int32_t yaw){
    int v;if(!adjusted(sar(w(pitch),1),v))return 2;sw(0xb13,s(0xb13)+w(v*2));
    if(!adjusted(sar(w(yaw),1),v))return 2;
    sw(0xb11,s(0xb11)+w(v*2));return 0;
}
extern "C" uint32_t cc_map_zoom(){
    int direction=0;if(key(0x51)&&u(0xb0f)<210)direction=1024;
    // Original MOV AX,oz+2 does not update flags: the following JZ preserves
    // the preceding key test and does not impose the comment's height gate.
    if(key(0x49))direction=-1024;
    if(direction){
        int32_t altitude=d((uint32_t(l(0xb0d))>>4)+40),delta=dwa(mul(altitude,direction)/1024);
        if(key(0x1f))delta=delta<0?-1000:1000;
        addl(0xb0d,delta);
    }
    if(s(0xb0f)<0)sl(0xb0d,0);
    return 0;
}
extern "C" uint32_t cc_map_controls(int32_t x,int32_t y){
    x=w(x);y=w(y);
    if(key(0x47)){int32_t z=l(0xb0d);cc_map_restore_observer();sw(0xb11,0);sw(0xb13,16384);sw(0xb15,0);matrices();sl(0xb0d,z);}
    if(key(0x2a)){unsigned result=cc_map_bump_angles(w(-y),w(-x));if(result)return result;matrices();return 0;}
    if(uint16_t(absw(x))<=32)x=0;
    if(uint16_t(absw(y))<=32)y=0;
    addl(0xb05,pan_amount(w(-y)));addl(0xb09,pan_amount(w(-x)));return cc_map_zoom();
}
extern "C" uint32_t cc_map_after_controls(){
    unsigned scan=b(0x26c)^128;if(!cc_runtime_state()[1]&&scan!=1&&scan!=0x1c&&scan!=0x32&&scan!=0x39)return 0;
    cc_map_end();finish_outer();return 1;
}
extern "C" void cc_map_draw_begin(){draw_tile=0;draw_index=0;draw_pending=false;}
extern "C" uint32_t cc_map_draw_next(){
    if(draw_pending){sw(f::tileptr,u(f::tileptr)+23);++draw_index;draw_pending=false;}
    while(draw_tile<64){
        if(draw_index>=150){++draw_tile;draw_index=0;continue;}
        const unsigned x=draw_tile%8,y=draw_tile/8;
        if(!draw_index){sw(f::curtilex,x);sw(f::curtiley,y);sw(f::tileseg,u(f::memseg)+0xff9+y*1728+x*216);sw(f::tileptr,0);}
        unsigned status=cc_decode_object(f::memobjbuf,x,y,draw_index);
        if(status==2)return UINT32_MAX;
        if(status){++draw_tile;draw_index=0;continue;}
        draw_pending=true;return f::memobjbuf;
    }
    return 0;
}
extern "C" uint32_t cc_map_marker(){wb(0x5b30,(raw_ticks()&512)?15:0);return u(0xb0f)>=(u(0x1efa)>>2)?0x5b66:0;}
