#include "camera.hpp"
#include "flight_lifecycle.hpp"
#include "state.hpp"
using namespace cc;using namespace cc::state;
extern "C" {
void cc_angles_matrix(const uint16_t*,uint16_t*,uint32_t);
void cc_matrix_angles(const uint16_t*,uint16_t*);
void cc_rotate_plane(uint16_t*,int32_t,int32_t,int32_t);
uint32_t cc_atn2(uint32_t,uint32_t);
}
namespace {
uint32_t palette=256;
bool clock_ok(){return s(0x1e5e)!=0&&s(0x1b7f)!=0;}
uint32_t magnitude32(int32_t v){return v<0?0u-uint32_t(v):uint32_t(v);}
unsigned relative(unsigned target){
    unsigned mask=0;
    for(unsigned i=0;i<3;++i){int32_t v=d(int64_t(l(target+i*4))-l(0xb05+i*4));sl(0xe7e + i*4,v);mask|=(magnitude32(v)*2u)>>16;}
    unsigned z0=u(0x1efa);int32_t above=w(-s(0xe88));
    if(z0&&above>=0&&unsigned(above)>z0){
        unsigned denominator=(magnitude32(l(0xe86))*16u)>>16,numerator=uint16_t(z0*16u);
        if(denominator>numerator){
            unsigned factor=((uint32_t(numerator)<<16)/denominator)>>1;sw(0x1efc,factor);mask=0;
            for(unsigned i=0;i<3;++i){int64_t product=int64_t(l(0xe7e + i*4))*factor;int64_t shifted=product>=0?(product/65536)*2:-((-product+65535)/65536)*2;sl(0xe7e + i*4,d(shifted));mask|=uint16_t(absw(w(uint64_t(product)>>32)));}
        }
    }
    return mask;
}
}
extern "C" uint32_t cc_camera_prepare(){
    if(!clock_ok())return 2;
    int32_t divisor=uwa(2);if(uint16_t(divisor)<2)divisor=2;
    for(unsigned i=0;i<2;++i){int32_t delta=w(s(0xf35+i*2)-s(0xf0b+i*2));if(delta)sw(0xf0b+i*2,s(0xf0b+i*2)+delta/divisor);}
    if(b(0xf3b)){
        uint16_t m[9],angles[3],forward[9];for(unsigned i=0;i<9;++i)m[i]=u(0x1ff0+i*2);
        cc_rotate_plane(m,s(0xf0f),s(0xf0d),s(0xf0b));cc_matrix_angles(m,angles);cc_angles_matrix(angles,forward,0);
        for(unsigned i=0;i<9;++i){sw(0xf23+i*2,m[i]);sw(0xf11+i*2,forward[i]);}for(unsigned i=0;i<3;++i)sw(0xf3d+i*2,angles[i]);
        sw(0x1ef6,0xf11);sw(0x1ef8,0xf23);
    }else{sw(0xf3f,s(0xb13));sw(0x1ef6,0x1fde);sw(0x1ef8,0x1ff0);}
    return 0;
}
extern "C" void cc_view_input(){
    if(b(0x26c)==0xc5){wb(0x26c,0);wb(0xae4,b(0xae4)^255);}
    for(unsigned i=0;i<3;++i)sw(0xf35+i*2,0);
    wb(0xf3b,0);
    if(!cc_lifecycle_key_down(0x1d)&&!b(0xae4)){for(unsigned i=0;i<3;++i)sw(0xf0b+i*2,0);return;}
    wb(0xf3b,255);
    if(cc_lifecycle_key_down(0x4b))sw(0xf35,16380);
    if(cc_lifecycle_key_down(0x4d))sw(0xf35,-16380);
    if(cc_lifecycle_key_down(0x48))sw(0xf37,-8192);
    if(cc_lifecycle_key_down(0x50)){if(b(0xafc))sw(0xf35,32767);else sw(0xf37,8192);}
    if(!b(0xafc)&&cc_lifecycle_key_down(0x4c))sw(0xf37,16383);
}
extern "C" void cc_view_remote_input(){if(b(0x26c)==0xba){wb(0x26c,0);wb(0xf54,b(0xf54)^255);}}
extern "C" uint32_t cc_camera_towards(uint32_t target){
    if(!clock_ok())return 2;
    if(relative(uint16_t(target))&0xfff0)return 1;
    for(unsigned i=0;i<3;++i)sw(0x1d06+i*2,w(-int32_t(uint32_t(l(0xe7e + i*4))>>6)));
    int32_t delta=w(int32_t(cc_atn2(u(0x1d06),u(0x1d08)))+32768-s(0xb11));
    sw(0xb11,s(0xb11)+wa(sar(delta,b(0x8f94)?7:4)));return 0;
}
extern "C" uint32_t cc_camera_after_world(){
    palette=256;if(!b(0xafc))return 0;
    if((b(0xf54)||b(0x8f94))&&!clock_ok())return 2;
    if(b(0xf54))cc_camera_towards(0x5bb0);
    if(!b(0x8f94))return 0;
    wb(0xf54,0);cc_camera_towards(0x8f96);sw(0xf6b,wa(500));cc_pilot_step(1);
    if(b(0x8f94)&128)return 0;
    int32_t rate=wa(s(0x272));if(!rate)rate=1;
    // Source SBB DX,-1 leaves DX either0 or1; its JNS always accepts the
    // wrapped low-word subtraction, even when fadecolor underflows.
    sw(0x270,u(0x270)-rate);if(!(b(0xf4b)&1))palette=b(0x271);
    return 0;
}
extern "C" uint32_t cc_camera_palette(){return palette;}
