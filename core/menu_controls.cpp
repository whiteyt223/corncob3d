#include "menu_controls.hpp"
extern "C" void cc_menu_controls_choice(uint16_t* p,uint32_t kind){
    switch(kind){case 0:p[0]=1;p[1]=0;break;case 1:p[0]=0;break;case 2:p[1]=1;break;case 3:p[2]=0;break;case 4:p[2]=1;break;}
}
extern "C" uint32_t cc_menu_controls_flags(const uint16_t* p,uint32_t invincible){
    return (p[2]?1u:0)|(uint16_t(invincible)?2u:0)|(p[1]?4u:0)|(p[0]?8u:0);
}
