#include "demo_frame.hpp"
#include "edition.hpp"
#include "audio.hpp"
#include "hud.hpp"
#include "cockpit_module.hpp"
#include "state.hpp"
#include "runtime.hpp"
using namespace cc;using namespace cc::state;
namespace f=cockpit_field;
extern "C" void cc_matvmul(const uint16_t*,const uint16_t*,uint16_t*);
namespace {
uint16_t temperature_ax;
bool quotient(uint32_t dividend,uint16_t divisor,uint16_t& out){
    if(!divisor)return false;
    uint32_t q=dividend/divisor;if(q>65535)return false;
    out=uint16_t(q);return true;
}
}
// 3.ASM:7978, original2FA0..2FC6. Preserve signed wrapped error term.
extern "C" uint32_t cc_cockpit_oil_update(){
    uint16_t q;if(!quotient(uint32_t(u(f::rpm))<<15,u(f::rpmx),q))return 2;
    uint32_t target=(uint32_t(q)*u(f::thrtmult))>>16;
    sw(f::oil,u(f::oil)+w(target-u(f::oil))/40);return 0;
}
// 3.ASM freeze1:308A..30A6. AX is deliberately not cleared in original.
extern "C" uint32_t cc_cockpit_temperature_update(uint32_t incoming_ax){
    uint16_t target;uint32_t dividend=((uint32_t(u(f::rpm))<<16)|uint16_t(incoming_ax))>>1;
    if(!quotient(dividend,u(f::rpmx),target))return 2;
    sw(f::temperature,u(f::temperature)+w(target-u(f::temperature))/180);return 0;
}
extern "C" uint32_t cc_cockpit_temperature_ax(){return temperature_ax;}
extern "C" uint32_t cc_cockpit_engine_update(){
    // Preserve the source audio block's numerical AX, wind timer and RNG
    // effects. ADL.ASM voiceon/voiceoff/adlwrt preserve AX; audio synthesis is
    // external. The engine update belongs after drawndles+flipage.
    uint16_t old_oil=u(f::oil);uint32_t status=cc_cockpit_oil_update();if(status)return status;
    uint16_t ax=uint16_t(u(f::oil)-old_oil);bool wind=false;
    if(b(f::remote)||!b(f::eject)){
        if(!u(f::currpmx)){ax=64;cc_audio_voice_off(0,0x40);wind=!u(f::crshvel);}
        else{
            ax=uint16_t(u(f::oil)+1920);
            if(s(f::speed)>=0){
                ax=uint16_t(clamp(int((uint32_t(ax)+u(f::speed))/80),64,1024));
                cc_audio_voice_on(0,ax);ax=uint16_t(u(f::engdam)*2u);
                if(w(ax)>=0){ax=uint16_t(16u-ax);if(w(ax)<0)ax=0;cc_audio_write(0x40,uint8_t(ax));}
            }
        }
    }else wind=true;
    if(wind){
        ax=u(f::windtick);
        if(w(ax-raw_ticks())<0){wb(f::windflag,0u-b(f::windflag));cc_audio_write(0xbd,(b(f::windflag)&0x80)?0x28:0x20);ax=uint16_t(raw_ticks()+random_bound(4096));sw(f::windtick,ax);}
    }
    temperature_ax=ax;
    if(b(f::frozen)){if(b(0xad8))cc_hud_button(12,(raw_ticks()&1536)?0x0701:0x0107);return 0;}
    return cc_cockpit_temperature_update(ax);
}
// 3.ASM:10233, original43C6..4422. This only updates the existing source
// object. Rendering should pass DS5BFA through the normal object renderer.
extern "C" void cc_cockpit_prop_update(){
    sw(f::propvec+4,-900);
    for(unsigned i=0;i<18;++i)wb(f::prop_object+i,b(f::pos+i));
    uint16_t matrix[9],vector[3],out[3];
    for(unsigned i=0;i<9;++i)matrix[i]=u(f::norot+i*2);
    for(unsigned i=0;i<3;++i)vector[i]=u(f::propvec+i*2);
    cc_matvmul(matrix,vector,out);
    for(unsigned i=0;i<3;++i){sw(f::xvec+i*2,out[i]);addl(f::prop_object+i*4,w(out[i]));}
    uint32_t spin=uint32_t(raw_ticks())*uint16_t((u(f::rpm)-10u)*2u);
    sw(f::prop_object+16,w(0u-spin));
}
extern "C" uint32_t cc_cockpit_prop_visible(){return cc_demo_frame_mode()!=255&&!b(f::eject)&&s(f::rpm)<=50&&u(f::currpmx)!=0;}
