#include "modal_controls.hpp"
#include "state.hpp"
#include "runtime.hpp"
#include "sortie.hpp"
#include "hud.hpp"
#include "audio.hpp"
#include "flight_lifecycle.hpp"
#include "late_weapons.hpp"
using namespace cc;using namespace cc::state;
extern "C" {
uint8_t* cc_video_memory();uint32_t cc_legacy_ticks();
uint32_t cc_sin(uint32_t);int32_t cc_ssin(uint32_t);
void cc_calcmat(const uint16_t*,uint16_t*);void cc_ncalcmat(const uint16_t*,uint16_t*);
}
namespace {
uint32_t palette[66]={};
void matrices(){
    uint16_t trig[6],matrix[9];
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<2;++j){unsigned angle=uint16_t(u(0xb11+i*2)+j*16384);wb(0xf378,cc_sin(angle)>>16);trig[i*2+j]=uint16_t(cc_ssin(angle));sw(0x20ae + i*4+j*2,trig[i*2+j]);}
    cc_calcmat(trig,matrix);for(unsigned i=0;i<9;++i)sw(0x1fde + i*2,matrix[i]);
    cc_ncalcmat(trig,matrix);for(unsigned i=0;i<9;++i)sw(0x1ff0+i*2,matrix[i]);
    for(unsigned i=0;i<3;++i)sw(0x20ae + i*4,-w(trig[i*2]));
}
}
extern "C" uint32_t cc_modal_radio_begin(uint32_t before,uint32_t after){
    if(cc_radio_state()[0])return 1;
    unsigned status=cc_modal_radio_draw_queued(before);
    if(status==1)cc_modal_radio_wait_start(after);
    return status;
}
extern "C" uint32_t cc_modal_radio_draw_queued(uint32_t before){
    if(cc_radio_state()[0])return 1;
    if(b(0x26c)==0xc4)sw(0xf7f,0x102f);
    if(!u(0xf7f)||!b(0xaee))return 0;
    cc_sortie_add_elapsed(before);
    return cc_radio_open(u(0xf7f),1);
}
extern "C" void cc_modal_radio_wait_start(uint32_t after){cc_sortie_timer_reset(after);}
extern "C" uint32_t cc_modal_tower_enter(uint32_t clock){
    // reset9's DX survives into cleargetkey's accidental setkbit fallthrough.
    for(unsigned i=0;i<16;++i)wb(0x2c7+i,0);
    wb(0x2c7,b(0x2c5));cc_sortie_add_elapsed(clock);
    return uint8_t(cc_runtime_state()[1])?0:1;
}
extern "C" void cc_modal_tower_audio(){cc_audio_tower();wb(0x19ba,0);wb(0x19bc,0);}
extern "C" uint32_t* cc_modal_tower_palette(){return palette;}
extern "C" void cc_modal_tower_return(uint32_t clock){
    wb(0xf3c,0);wb(0xf50,255);wb(0x26c,0);
    if(b(0xafc)&&!b(0xf4d)&&b(0x1ca)!=1){
        addl(0xb09,-2000);sw(0xb11,-16384);matrices();
        if(u(0x5bc8)){
            sw(0x5bc8,1);for(unsigned i=0;i<3;++i)sw(0x5bd8+i*2,0x4aa9);
            sw(0x5bbe,-1500);sw(0x5bb8,u(0xf5c));
        }
        wb(0xf7a,0);cc_aircraft_clear_damage();sw(0x19f6,cc_legacy_ticks());sw(0x19f4,2184);
    }
    wb(0x1ca,0);
    // BIOS mode10 reinitializes the graphics device. Its framebuffer clear is
    // the platform counterpart of grmode, not an original gameplay routine.
    for(unsigned i=0;i<65536*8;++i)cc_video_memory()[i]=0;
    ++palette[0];palette[1]=63;
    for(unsigned i=0;i<16;++i){
        unsigned p=0x1e6e + i*4;palette[2+i*4]=b(p);
        for(unsigned j=0;j<3;++j){
            unsigned value=b(p+1+j)*255u>>8;
            if(b(0xf4b)&1){unsigned sum=uint8_t(b(p+1+j)+b(0x1eef+j*2));value=(sum&64)?63-(sum&63):sum&63;wb(p+65+j,value);}
            palette[3+i*4+j]=value;
        }
    }
    wb(0x1c9,255);wb(0x26c,0);cc_sortie_timer_reset(clock);
}
extern "C" void cc_modal_tower_finish(){wb(0xf4d,0);cc_weapon_restore_view();}
