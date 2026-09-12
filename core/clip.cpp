#include "viewport.hpp"
#include <stdint.h>

// F3DVEC.ASM polyhell/choplin, fixed main-window near plane window(Zx).
// Preserve original traversal from the minimum-depth vertex, integer division,
// and the plane+1 depth adjustment. These routines expect convex input.
namespace {
using namespace cc::view;
uint16_t input[126],output[66];uint32_t output_count=0;
int32_t s(uint16_t v){return v<32768?int32_t(v):int32_t(v)-65536;}
int32_t w(int32_t v){return s(uint16_t(v));}
void copy(uint16_t* d,const uint16_t* a){for(int i=0;i<3;++i)d[i]=a[i];}
uint32_t chop(uint16_t* a,uint16_t* b){
    int32_t za=s(a[0]),zb=s(b[0]);
    uint16_t da=uint16_t(za-window(Zx)),db=uint16_t(zb-window(Zx));
    if((da&db)&32768)return 1;
    if(!((da|db)&32768))return 0;
    if(!(uint16_t(za-zb)&32768)){
        for(int i=0;i<3;++i){auto tmp=a[i];a[i]=b[i];b[i]=tmp;}
        za=s(a[0]);zb=s(b[0]);
    }
    if(!(uint16_t(window(Zx)-zb)&32768))return 1;
    int32_t fraction=w(window(Zx)-za),divisor=w(zb-za);
    if(fraction<0)return 1;
    if(!divisor)return 2;
    for(int i=1;i<3;++i){
        int32_t quotient=(w(s(b[i])-s(a[i]))*fraction)/divisor;
        if(quotient < -32768 || quotient>32767)return 2;
        a[i]=uint16_t(uint32_t(a[i])+uint32_t(quotient));
        if(i==1)a[0]=window(Zx)+1;
    }
    return 0;
}
}
extern "C" uint16_t* cc_clip_input(){return input;}
extern "C" uint16_t* cc_clip_output(){return output;}
extern "C" uint32_t cc_clip_count(){return output_count;}
extern "C" uint32_t cc_clip_polygon(uint32_t count){
    output_count=0;
    if(count<3||count>20)return 1;
    int minimum=32767,maximum=-32767,index=0;
    for(unsigned i=0;i<count;++i){int z=s(input[i*3]);if(z<minimum){minimum=z;index=int(i);}if(z>maximum)maximum=z;}
    if(maximum<=window(Zx))return 1;
    if(minimum>=window(Zx)){for(unsigned i=0;i<count*3;++i)output[i]=input[i];output_count=count;return 0;}
    for(int i=0;i<(index+1)*3;++i)input[count*3+unsigned(i)]=input[i];
    unsigned scanned=0;
    do{index=(index+1)%int(count);if(++scanned>=count)return 1;}while(s(input[index*3])<window(Zx));
    copy(output,input+((index+int(count)-1)%int(count))*3);copy(output+3,input+index*3);
    auto status=chop(output,output+3);if(status)return status;
    unsigned written=2;
    for(;;){
        index=(index+1)%int(count);if(++scanned>count)return 1;
        if(written>=22)return 1;
        copy(output+written*3,input+index*3);
        if(s(input[index*3])<window(Zx)){
            auto* previous=output+(written-1)*3;auto* current=output+written*3;
            status=chop(previous,current);if(status)return status;
            for(int i=0;i<3;++i){auto tmp=previous[i];previous[i]=current[i];current[i]=tmp;}
            output_count=written+1;return 0;
        }
        ++written;
    }
}
