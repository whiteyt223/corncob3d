#include "demo_frame.hpp"
#include "edition.hpp"
#include "pause.hpp"
#include "sortie.hpp"
#include "flight_lifecycle.hpp"
#include "hud.hpp"
#include "audio.hpp"
#include "runtime.hpp"
#include "state.hpp"
using namespace cc;using namespace cc::state;
extern "C" uint8_t* cc_video_memory();
namespace {
unsigned phase=0,flash=0x070f,pending=0;uint8_t help_image[640*350];
unsigned wait_tail(unsigned clock){
    // Every original busy-loop iteration swaps if raw time's low10 bits are0.
    // Do not replace this with a different elapsed-time blink formula.
    if(!(raw_ticks()&1023))flash=((flash&255)<<8)|(flash>>8);
    cc_hud_button(1,flash);
    if(b(0x26c)!=0x99&&b(0x26c)!=0x81)return phase=1;
    cc_sortie_timer_reset(clock);wb(0x26c,0);cc_hud_button(1,0x0005);
    if((flash>>8)==15){flash=((flash&255)<<8)|(flash>>8);cc_hud_button(1,flash);}
    return phase=0;
}
unsigned request_image(){
    wb(0x26c,0);pending=b(0xb2f+b(0xb2e));sw(0xd2a,0);
    unsigned cursor=0xd2a;if(pending>=100)wb(cursor++,48+pending/100);
    if(pending>=10)wb(cursor++,48+(pending/10)%10);
    wb(cursor++,48+pending%10);wb(cursor,0);
    return phase=2;
}
}
extern "C" uint32_t cc_frame_input_prefix(uint32_t raw){
    cc_demo_frame_begin();
    sw(0x1a1a,u(0x1a1a)+1);raw=uint16_t(raw);
    if(u(0x19be)){raw=u(0x19be);cc_runtime_state()[0]=uint16_t(raw_ticks()+raw);}
    int value=raw>>3;if(value>=s(0xe51))value=s(0xe51);if(value<=s(0xe53))value=s(0xe53);
    wb(0xae9,0);sw(0x1e5e,value);sw(0x1e60,value);
    if(cc_lifecycle_key_down(0x14)){sw(0x1e5e,50);wb(0xae9,255);}
    cc_demo_frame_timing();
    if(b(0x26c)==0xc4)sw(0xf7f,0x102f);
    return u(0x1e5e);
}
extern "C" void cc_pause_reset(){phase=0;flash=0x070f;pending=0;}
extern "C" uint32_t cc_pause_stage(){return phase;}
extern "C" uint32_t cc_pause_image_id(){return pending;}
extern "C" uint8_t* cc_pause_image_buffer(){return help_image;}
extern "C" uint32_t cc_pause_flash_ax(){return flash;}
extern "C" uint32_t cc_pause_begin(uint32_t clock){
    if(phase)return phase;
    if(b(0xe46)||b(0x26c)==0xbe)wb(0x26c,0xb9);else if(b(0x26c)!=0x99)return 0;
    if(cc_lifecycle_key_down(0x38))return 0;
    cc_audio_pause();
    if(!b(0xe46)&&b(0x26c)!=0xb9)wb(0x26c,0);
    wb(0xe46,0);wb(0xb2e,b(0xf77)?0:1);cc_sortie_add_elapsed(clock);
    cc_hud_button(1,0x0f07);cc_hud_button(1,0x0500);flash=0x070f;phase=1;
    return cc_pause_update(clock);
}
extern "C" uint32_t cc_pause_update(uint32_t clock){
    if(phase!=1)return phase;
    if(b(0x26c)==0xb9)return request_image();
    return wait_tail(clock);
}
extern "C" uint32_t cc_pause_after_image(uint32_t failed,uint32_t clock){
    if(phase!=2)return phase;
    if(failed)source_error(40);
    else for(unsigned i=0;i<199*640;++i){cc_video_memory()[i]=help_image[i];cc_video_memory()[0x7e00*8+i]=help_image[i];}
    wb(0xb2e,b(0xb2e)+1);if(b(0xb2e)>9)wb(0xb2e,0);
    return wait_tail(clock);
}

extern "C" void cc_frame_after_pause(){
    if(u(0xafa)==u(0xb17)){sw(0xafa,u(0xafa)+0x151);sw(0xb17,u(0xb17)-0x151);}
}
