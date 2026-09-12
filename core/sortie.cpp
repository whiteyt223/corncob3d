#include "edition.hpp"
#include "sortie.hpp"
#include "flight_lifecycle.hpp"
#include "runtime.hpp"
#include "state.hpp"
using namespace cc;using namespace cc::state;
namespace {
unsigned phase=0,beep=0,palette=256;
unsigned cleanup(unsigned ticks){sw(0x1a1c,ticks);return phase=2;}
bool exit_requested(){
    if(b(0x26c)==0x81){
        if(!b(0xaee)||b(0xf52)||b(0xe47))return true;
        wb(0xe47,255);sw(0xf7f,b(0xafc)?0x174d:0x15f8);
    }
    if(b(0x19ec))return true;
    wb(0x19ed,255);
    if(cc_lifecycle_key_down(0x1d)&&cc_lifecycle_key_down(0x2e))return true;
    wb(0x19ed,0);
    if(uint8_t(cc_runtime_state()[1]))return true;
    if(b(0x8f95)){wb(0x8f94,0);return true;}
    if(b(0xaf2)){sw(0x1e4,u(0x1e4)|8);return true;}
    return false;
}
unsigned world_gate(){return phase=(uint8_t(cc_runtime_state()[1])==0&&b(0xaf1)?4:5);}
unsigned clock_value(uint32_t v){return (v&255)+100*((v>>8&255)+60*((v>>16&255)+60*(v>>24)));}
uint32_t packed(unsigned v){unsigned hundredths=v%100;v/=100;unsigned seconds=v%60;v/=60;unsigned minutes=v%60;v/=60;return hundredths|(seconds<<8)|(minutes<<16)|(v<<24);}
}
extern "C" void cc_sortie_reset(){phase=0;beep=0;palette=256;}
extern "C" uint32_t cc_sortie_stage(){return phase;}
extern "C" uint32_t cc_sortie_beep(){return beep;}
extern "C" uint32_t cc_sortie_take_beep(){const auto value=beep;beep=0;return value;}
extern "C" void cc_sortie_requests(){
    beep=0;
    if(b(0x26c)==0xb9)cc_aircraft_pull_chute();
    if(b(0x26c)==0xa6){wb(0x26c,0);cc_aircraft_fullsize();}
    if(b(0x26c)==0xbb){wb(0x26c,0);wb(0xf4d,255);}
    if(b(0x26c)==0xc0){wb(0x26c,0);wb(0xf4d,0x7d);}
    if(b(0x26c)==0xbf){wb(0x26c,0);if(b(0xaf6)){wb(0x1baa,255);wb(0xf4d,255);}else beep=1;}
    if(b(0x26c)==0xbd){wb(0x26c,0);wb(0xf4d,0x7f);}
    if(u(0x1bc8)==7){if(!b(0xaf1))sw(0xf7f,0xfdc);else if(u(0x1de2)==20)sw(0xf7f,0xf83);}
    if(b(0x26c)==0xbc){wb(0x26c,0);wb(0x1ca,1);}
    if(cc_lifecycle_key_down(0x23)&&b(0xafc))wb(0x8f94,255);
}
extern "C" uint32_t cc_sortie_frame_end(uint32_t ticks){
    if(phase)return phase;
    cc_sortie_requests();return exit_requested()?cc_sortie_begin_exit(ticks):0;
}
extern "C" uint32_t cc_sortie_begin_exit(uint32_t ticks){
    if(phase)return phase;
    // Forced ejection re-enters only the late request tail before donep33.
    for(;;){
        if(b(0x19ed)){sw(0x1e4,u(0x1e4)|0x4000);return cleanup(ticks);}
        if(!b(0xae7)&&b(0xae6))sw(0x1e4,u(0x1e4)|0x2000);
        if(!b(0x19ec)){
            sw(0x1e4,u(0x1e4)&~1);
            if(b(0xf55))sw(0x1e4,u(0x1e4)|(b(0xf56)?1:0x10));
            if(b(0x8f95))sw(0x1e4,(u(0x1e4)|0x100)&~8);
            if(b(0xaf1)&&!b(0xd5c)&&!b(0xe58)&&!b(0xafc)&&!b(0x19ed)){
                wb(0x19ec,255);wb(0x26c,0);cc_aircraft_eject();cc_sortie_requests();exit_requested();continue;
            }
        }
        if(!u(0x5bc8))return cleanup(ticks);
        sw(0x5bf8,2);sw(0x5bc8,u(0x5bc8)|u(0x1de)|0x10);sw(0x5be4,32000);
        return phase=1;
    }
}
extern "C" uint32_t cc_sortie_after_near(uint32_t ticks){return phase==1?cleanup(ticks):phase;}
extern "C" void cc_sortie_config_hash(){
    unsigned sum=random_word()|0x5000;sw(0xb25,sum);sw(0xb27,random_word());
    for(unsigned p=0xaf7;p<0xb25;p+=2){sum=uint16_t(sum+(uint8_t(cc_runtime_state()[1])==97?0:u(p)));sw(p,sum);}
}
extern "C" void cc_sortie_config_unhash(){
    unsigned sum=u(0xb25);for(unsigned p=0xaf7;p<0xb25;p+=2){unsigned next=u(p);sw(p,next-sum);sum=next;}
}
extern "C" uint32_t cc_sortie_after_config(){
    if(phase!=2)return phase;
    sw(0x26a,uint8_t(cc_runtime_state()[1]));
    if(!b(0xaf0))return world_gate();
    sw(0x1e8,clamp(w(s(0x1e8)-s(0xf0fc)),0,32767));sw(0xf0fc,0);if(!cc_edition_is_deluxe())sw(0x1e4,u(0x1e4)&u(0x1e2));return phase=3;
}
extern "C" uint32_t cc_sortie_after_results(){return phase==3?world_gate():phase;}
extern "C" uint32_t cc_sortie_after_world(){if(phase==4)phase=5;return phase;}
extern "C" void cc_sortie_timer_reset(uint32_t clock){sl(0xf71e,d(clock));}
extern "C" uint32_t cc_sortie_timer_read(uint32_t clock){unsigned now=clock_value(clock),start=clock_value(uint32_t(l(0xf71e)));if(now<start)now+=8640000;return packed(now-start);}
extern "C" uint32_t cc_sortie_add_elapsed(uint32_t clock){uint32_t elapsed=cc_sortie_timer_read(clock);unsigned seconds=((elapsed>>16)&255)*60+((elapsed>>8)&255);sw(0x252,u(0x252)+seconds);return seconds;}
extern "C" uint32_t cc_sortie_dvel_average(){
    palette=256;if(u(0x1bc8)<=3)return 0;
    if(!s(0x1e5e))return 2;
    int32_t divisor=uwa(s(0xf73));int32_t avg=d((uint32_t(u(0xf6f))<<16)|u(0xf71));
    int32_t delta=d(int64_t(b(0xafc)?0:l(0x1d98))-avg);avg=d(int64_t(avg)+rdiv(delta,divisor));
    sw(0xf71,avg);sw(0xf6f,clamp(hi(avg),-100,100));int32_t a=absw(s(0xf6f))-60;
    if(a>=0){wb(0x271,255-unsigned(a)*255/41);if(!(b(0xf4b)&1))palette=b(0x271);}
    else if(b(0x271)!=255){wb(0x271,255);if(!(b(0xf4b)&1))palette=255;}
    return 0;
}
extern "C" uint32_t cc_sortie_palette(){return palette;}
