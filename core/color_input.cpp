#include "color_input.hpp"
#include "flight_lifecycle.hpp"
#include "state.hpp"
using namespace cc;using namespace cc::state;
namespace {
unsigned waiting=0,dirty=0;
void distort(){
    for(unsigned i=0;i<16;++i)for(unsigned channel=0;channel<3;++channel){
        unsigned sum=uint8_t(b(0x1e6e + i*4+1+channel)+b(0x1eef+channel*2));
        wb(0x1eae + i*4+1+channel,(sum&64)?63-(sum&63):(sum&63));
    }
    dirty=1;
}
void select_color(bool sky){
    for(unsigned i=0;i<3;++i)wb((sky?0x1e6b:0x1e68)+i,b((sky?0x1edb:0x1eb7)+i));
    for(unsigned i=0;i<3;++i){wb(0x1e77+i,b(0x1e68+i));wb(0x1e9b+i,b(0x1e6b+i));}
    // getkbit's BX=0 for keys1/2, so packbits' two retained high bits are0.
    uint32_t bits=0;for(unsigned i=0;i<5;++i)bits=(bits<<6)|(b((i<3?0x1e6b:0x1e65)+i)&63);
    for(unsigned i=0;i<4;++i)wb(0x1fae + i,bits>>(24-i*8));
    wb(0x1fb2,b(0x1e6a)<<2);
}
unsigned ground(){if(cc_lifecycle_key_down(3)){select_color(false);return waiting=2;}return waiting=0;}
}
extern "C" void cc_color_input_reset(){waiting=0;dirty=0;}
extern "C" uint32_t cc_color_input_palette_dirty(){return dirty;}
extern "C" uint32_t cc_color_input(){
    dirty=0;
    if(waiting==1){if(cc_lifecycle_key_down(2))return 1;return ground();}
    if(waiting==2){if(cc_lifecycle_key_down(3))return 2;return waiting=0;}
    wb(0xf4b,(u(0x1eee)|u(0x1ef0)|u(0x1ef2))?1:0);
    if(!cc_lifecycle_key_down(0x23))return 0;
    if(!s(0x1b7f))return 3;
    int32_t rate=s(0x276)*s(0x1e5e)/s(0x1b7f);if(rate<-32768||rate>32767)return 3;sw(0x278,rate);
    const unsigned keys[]={0x13,0x22,0x30};for(unsigned i=0;i<3;++i)if(cc_lifecycle_key_down(keys[i]))sw(0x1eee + i*2,u(0x1eee + i*2)+rate);
    distort();
    if(cc_lifecycle_key_down(2)){select_color(true);return waiting=1;}
    return ground();
}
