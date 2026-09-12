#include "ground_stars.hpp"
#include "state.hpp"
#include "viewport.hpp"
using namespace cc;using namespace cc::state;
extern "C" {
void cc_pmvmul(const uint16_t*,const uint16_t*,uint16_t*);
uint32_t cc_view_point_front(uint32_t);
}
namespace {
uint32_t stats[2];uint8_t buffer[4566];
unsigned getw(const uint8_t*p,unsigned at){return p[at]|unsigned(p[at+1])<<8;}
int32_t getd(const uint8_t*p,unsigned at){return d(getw(p,at)|(uint32_t(getw(p,at+2))<<16));}
void putw(uint8_t*p,unsigned at,unsigned v){p[at]=uint8_t(v);p[at+1]=uint8_t(v>>8);}
void putd(uint8_t*p,unsigned at,uint32_t v){putw(p,at,v);putw(p,at+2,v>>16);}
void regenerate(uint8_t*p){
    if(l(0xb0d)<0)return;
    uint32_t extent=uint32_t(l(0xb0d))<<6;int32_t half=sar(d(extent),1);
    for(unsigned i=0;i<2;++i)putd(p,i*4,uint32_t(l(0xb05+i*4))+uint32_t(uint64_t(extent)*random_word()>>16)-uint32_t(half));
    putd(p,8,0);putw(p,12,0);
}
}
extern "C" uint32_t cc_g3dobj(const uint8_t* record,uint32_t star){
    sw(0x2063,getw(record,12));unsigned mask=0;int32_t delta[3];
    for(unsigned i=0;i<3;++i){
        int32_t value=d(int64_t(getd(record,i*4))-l(0xb05+i*4));if(star)value=sar(value,8);
        delta[i]=value;sl(0xe7e + i*4,value);uint32_t magnitude=value<0?0u-uint32_t(value):uint32_t(value);mask|=uint16_t(magnitude>>15);
    }
    if(mask&0xff00)return 1;
    unsigned shift=(mask&0xf0)?8:(mask&15)?4:0;
    if(shift)wb(0x205d,shift); // close path deliberately leaves rdf unchanged
    uint16_t relative[3],forward[9],origin[3];unsigned pointer=u(0x1ef6);
    for(unsigned i=0;i<3;++i){relative[i]=uint16_t(sar(w(sar(delta[i],shift)),1));sw(0x1d06+i*2,relative[i]);}
    for(unsigned i=0;i<9;++i)forward[i]=u(pointer+i*2);
    cc_pmvmul(forward,relative,origin);sw(0x205f,0x204c);wb(0x1b8b,b(0x1bae));
    for(unsigned i=0;i<3;++i){sw(0x204c+i*2,origin[i]);cc_views_input()[i]=origin[i];}
    if(!u(0x2063))return cc_view_point_front(b(0x1bae));
    unsigned status=cc_view_point(b(0x1bae));
    for(unsigned i=0;i<3;++i)sw(0x204c+i*2,cc_views_input()[i]);
    // Source CMP ptrgwint,0 clears carry when rear is disabled.
    return status?status:u(0x1c40)?cc_views_result()[1]:0;
}
extern "C" uint8_t* cc_ground_buffer(){return buffer;}
extern "C" uint32_t* cc_ground_stats(){return stats;}
extern "C" uint32_t cc_ground_frame(uint8_t* tables,uint32_t advance){
    stats[0]=stats[1]=0;if(!b(0x1bac))return 0;
    unsigned count=getw(tables,0);if(!count||count>80)return 3;
    for(unsigned i=0;i<count;++i){
        auto* record=tables+2+i*14;unsigned status=cc_g3dobj(record,0);if(status>1)return status;++stats[0];stats[1]+=status;
        if(advance){if(status)regenerate(record);if(random_word()<=91)regenerate(record);}
    }
    if(!b(0x1bad))return 0;
    unsigned saved=b(0x1bae);wb(0x1bae,15);
    for(unsigned i=0;i<count;++i){unsigned status=cc_g3dobj(tables+0x8fa+i*14,1);if(status>1){wb(0x1bae,saved);return status;}++stats[0];stats[1]+=status;}
    wb(0x1bae,saved);return 0;
}
extern "C" void cc_ground_input(){if(b(0x26c)==0xa2){wb(0x26c,0);wb(0x1bac,b(0x1bac)^255);}}
extern "C" void cc_world_horizon(){
    unsigned inverse=u(0x1ef8);int32_t y=sar(1300*s(inverse+6),15),x=sar(1300*s(inverse),15);
    sw(0x206b,y);sw(0x206d,x);sl(0x1c73,d(int64_t(l(0xb09))+y));sl(0x1c6f,d(int64_t(l(0xb05))+x));sl(0x1c77,l(0xb0d));
    sw(0x1ce1,y);sw(0x1cdb,-y);sw(0x1cdd,x);sw(0x1ce3,-x);int32_t pitch=b(0xf40);if(pitch>=128)pitch-=256;wb(0x205e,-sar(pitch,1));
}
