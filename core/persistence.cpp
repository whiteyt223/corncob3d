#include "state.hpp"
using namespace cc;using namespace cc::state;
extern "C" uint32_t cc_new_effect(uint32_t);
// Original DEFOBJS funeral5d (3.42 CS940A). adjustxy is RET in this
// executable. The failed strtperm path still mutates the template addressed
// by expobj; this awkward allocation-failure behavior is preserved.
extern "C" uint32_t cc_world_funeral(uint32_t destroyed){
    constexpr unsigned plane=0x5bb0,buffer=0x1a1e,prototype=0x7e6e;
    sw(plane+24,0);for(unsigned i=0;i<18;++i)wb(buffer+i,b(plane+i));
    for(unsigned p=prototype+46;p<=prototype+50;p+=2)sw(p,0);
    sw(0xec30,0);sw(0x8fa2,buffer);unsigned p=cc_new_effect(2);
    if(p){wb(p+18,0);sw(p+24,u(p+24)|0x8000);for(unsigned i=0;i<18;++i)wb(p+i,b(buffer+i));}
    p=u(0xec28);sw(p+24,u(p+24)|u(0x1de));wb(p+65,b(p+65)|0x10);
    if(destroyed)sw(p+24,(u(p+24)|0x100)&~8u);
    return p;
}
