#include "flight_lifecycle.hpp"
extern "C" uint32_t cc_world_funeral(uint32_t);
namespace {
uint32_t transition(uint32_t kind,uint32_t object,uint32_t detail){
    if(kind!=1||object!=0x5bb0)return 0;
    cc_world_funeral(detail);return 1;
}
}
extern "C" void cc_connect_lifecycle_world(){cc_lifecycle_set_hook(transition);}
