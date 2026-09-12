#include "pilot_menu.hpp"
extern "C" void cc_options_context(uint8_t*o,uint32_t combat){o[7]=uint8_t((o[7]&254)|((o[7]&(combat?8:2))?1:0));}
extern "C" uint32_t cc_options_toggle(uint8_t*o,uint32_t kind){
 switch(kind){case 0:o[7]^=32;break;case 1:o[7]^=64;break;
 case 2:o[7]|=16;o[7]=uint8_t((o[7]&8)?(o[7]&246):(o[7]|9));break;
 case 3:o[7]|=4;o[7]=uint8_t((o[7]&2)?(o[7]&252):(o[7]|3));break;default:return 1;}return 0;
}
extern "C" void cc_options_defaults(uint8_t*o){const uint8_t a[]={5,6,2,0,0,4,0,2};for(unsigned i=0;i<8;i++)o[i]=a[i];}
extern "C" uint32_t cc_options_set_enemy(uint8_t*o,uint32_t field,uint32_t value){if(field>5||value>(field<3?25u:8u))return 1;o[field]=uint8_t(value);return 0;}
extern "C" uint32_t cc_launch_warning_flags(uint32_t active,uint32_t planes,uint32_t local,uint32_t total){if(!active)return 0;return (uint16_t(planes)==0?2u:0u)|(uint16_t(total)==0?4u:uint16_t(local)==0?1u:0u);}
extern "C" int32_t cc_launch_warning_choice(uint32_t kind,uint32_t stores,uint32_t key){
 key=uint16_t(key);if(kind==2&&int16_t(stores)>0&&(key|32)==112)return 2;
 if(key==27)return -1;
 if(key==10||key==13)return 1;
 return 0;
}

extern "C" void cc_options_select_training(uint8_t*p){p[0x23d]|=128;cc_options_context(p+0x234,0);}
