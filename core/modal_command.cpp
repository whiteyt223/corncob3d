#include "modal_command.hpp"
#include "state.hpp"
using namespace cc;using namespace cc::state;
extern "C" void cc_modal_tower_command(uint32_t argument){
    // Actual shareware3.EXE BAC..BDB formats arguments even for F6's notice.
    if(b(0xf4d)==0x7f)return;
    uint32_t value=uint16_t(argument);unsigned base=10;
    if(b(0xf4d)==0x7d){sw(0x1f97,0);sw(0x1f99,0);value=uint32_t(uint16_t(u(0xd5e)+0xff9))<<16;base=16;}
    char reverse[10];unsigned n=0,p=0xd2a;
    do{unsigned digit=value%base;reverse[n++]=char(digit<10?'0'+digit:'A'+digit-10);value/=base;}while(value);
    while(n)wb(p++,reverse[--n]);
    wb(p,0);
}
