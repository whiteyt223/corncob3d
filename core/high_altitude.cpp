#include "high_altitude.hpp"
#include "edition.hpp"
#include "sortie.hpp"
#include "runtime.hpp"
#include "state.hpp"
using namespace cc;using namespace cc::state;
namespace {unsigned phase=0;}
extern "C" void cc_high_altitude_reset(){phase=0;}
extern "C" uint32_t cc_high_altitude_stage(){return phase;}
extern "C" uint32_t cc_high_altitude_begin(){
    if(phase)return phase;
    // This unaligned signed word is effective Z bits8..23; keep original
    // comparison instead of converting to feet or using the full altitude.
    // Native bodies: shareware checks protected-airspace and aceflag; Other
    // Worlds begins with only the signed effective-height comparison.
    if(s(0xe94)<s(0x27c))return 0;
    if(!cc_edition_is_other_worlds()&&(!(u(0x1de)&0x2000)||!b(0xaec)))return 0;
    wb(0xd5c,1);return phase=1;
}
extern "C" uint32_t cc_high_altitude_after_flush(){if(phase==1)phase=2;return phase;}
extern "C" uint32_t cc_high_altitude_after_near(){if(phase==2)phase=3;return phase;}
extern "C" uint32_t cc_high_altitude_after_presentation(uint32_t clock){
    if(phase!=3)return phase;
    // reset9 leaves DX=old interrupt9 offset. cleargetkey accidentally falls
    // through into setkbit with AX=0, restoring DL in the first cleared byte.
    for(unsigned i=0;i<16;++i)wb(0x2c7+i,0);
    wb(0x2c7,b(0x2c5));cc_sortie_add_elapsed(clock);
    sw(0xd2a,0);return phase=4; // runstars' empty environment/argument buffer
}
extern "C" uint32_t cc_high_altitude_after_stars(uint32_t clock,uint32_t failed,uint32_t raw_ticks){
    if(phase!=4)return phase;
    if(failed)source_error(61);
    cc_sortie_timer_reset(clock);wb(0x19ec,255);sw(0x1e4,u(0x1e4)|0x400);
    cc_sortie_begin_exit(raw_ticks);return phase=5;
}
