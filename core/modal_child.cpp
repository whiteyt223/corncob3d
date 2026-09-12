#include "modal_child.hpp"
#include "modal_command.hpp"
#include "edition.hpp"
#include "runtime.hpp"
#include "state.hpp"
using namespace cc;using namespace cc::state;
namespace { unsigned phase=0,kind=0; }
extern "C" void cc_modal_child_reset(){phase=0;kind=0;}
extern "C" uint32_t cc_modal_child_stage(){return phase;}
extern "C" uint32_t cc_modal_child_kind(){return kind;}
extern "C" uint32_t cc_modal_child_begin(){
    if(phase&&phase!=11)return phase;
    if(b(0x1baa)){kind=3;return phase=1;}
    const unsigned boss=b(0xf4d);
    cc_modal_tower_command(boss?0xffff:u(0x1cb));
    kind=boss==0x7f?1:boss==0x7d?2:0;
    if(kind==1)return phase=cc_edition_is_other_worlds()?7:8;
    if(kind==2)return phase=cc_edition_is_other_worlds()?2:9;
    return phase=7;
}
extern "C" uint32_t cc_modal_child_advance(){
    if(phase>=2&&phase<=6){if(phase==6)wb(0x1a1e,0);return ++phase;}
    if(phase>=8&&phase<=10)phase=11;
    return phase;
}
extern "C" const char* cc_modal_child_program(){
    if(phase!=7)return "";
    return kind==1?"iscore.exe":kind==2?"tu.exe":"tower.exe";
}
extern "C" uint32_t cc_modal_child_command(){return phase==7?(kind==2?0x1a1e:0xd26):0;}
extern "C" uint32_t cc_modal_child_exec_result(uint32_t carry,uint32_t al){
    if(phase!=7)return phase;
    if(carry||uint8_t(al)){source_error(61);return phase=11;}
    return phase=b(0x1ca)&&!b(0xafc)?10:11;
}
extern "C" uint32_t cc_modal_child_calibration_result(uint32_t error){
    if(phase!=1)return phase;
    source_error(uint8_t(error)==93?0:error);wb(0x1baa,0);return phase=11;
}
