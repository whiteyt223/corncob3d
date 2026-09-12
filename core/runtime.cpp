#include "audio.hpp"
#include "state.hpp"
namespace {uint32_t platform[3];}
extern "C" uint32_t* cc_runtime_state(){return platform;}
extern "C" void cc_clock_advance(uint32_t ticks){
    // Original raw clock and weapon cooldown portion of new08. Audio timer
    // synthesis and DOS interrupt chaining are browser platform adapters.
    ticks&=65535;
    for(unsigned i=0;i<ticks;++i){platform[0]=uint16_t(platform[0]+1);cc_audio_clock_advance(1);cc_audio_timer_tick();if(!(platform[0]&63)){platform[2]=uint16_t(platform[2]+1);
        for(unsigned p=0x1c9f;p<=0x1ca7;p+=8)if(cc::state::u(p))cc::state::sw(p,cc::state::u(p)-1);
    }}
}

extern "C" uint32_t cc_legacy_ticks(){return platform[2];}
