#include "demo.hpp"
namespace {
unsigned u(const uint8_t* p,unsigned a){return p[a]|(unsigned(p[a+1])<<8);}
int s(unsigned a){a&=65535;return a<32768?int(a):int(a)-65536;}
void w(uint8_t* p,unsigned a,unsigned v){p[a]=uint8_t(v);p[a+1]=uint8_t(v>>8);}
uint32_t d(const uint8_t* p,unsigned a){return u(p,a)|(uint32_t(u(p,a+2))<<16);}
void l(uint8_t* p,unsigned a,uint32_t v){w(p,a,v);w(p,a+2,v>>16);}
void copy(uint8_t* dst,const uint8_t* src,unsigned n){for(unsigned i=0;i<n;++i)dst[i]=src[i];}
unsigned absword(unsigned value){const int v=s(value);return uint16_t(v<0?-v:v);}
}
extern "C" uint32_t cc_demo_compact(uint8_t* state,const uint8_t* observer,uint8_t* output){
    state[0]|=128;output[0]=state[0];output[7]=0;
    for(unsigned i=0;i<3;++i){
        output[1+i]=state[31+i*2];output[4+i]=observer[13+i*2];
        output[7]|=uint8_t(((observer[12+i*2]>>6)&3)<<(4-i*2));
    }
    return 8;
}
extern "C" uint32_t cc_demo_decode(uint8_t* state,const uint8_t* input,uint32_t size,uint32_t ejected){
    if(!size)return UINT32_MAX;
    const unsigned flag=input[0];
    if(flag==255){state[0]=255;state[37]=0;return 1u<<16;}
    if(!(flag&8))return (1u<<16)|25;
    const unsigned count=(flag&128)?8:37;
    if(size<count)return UINT32_MAX;
    if(!(flag&128)){copy(state,input,(uint8_t(ejected)||(flag&32))?1:37);return count<<16;}
    state[0]=uint8_t(flag);if(uint8_t(ejected))return count<<16;
    for(unsigned i=0;i<3;++i){
        int acceleration=input[1+i];if(acceleration>=128)acceleration-=256;
        w(state,31+i*2,unsigned(acceleration));const int delta=s(u(state,19+i*4)+acceleration);
        w(state,19+i*4,unsigned(delta));l(state,1+i*4,d(state,1+i*4)+uint32_t(delta));
    }
    for(unsigned i=0;i<3;++i)w(state,13+i*2,(unsigned(input[4+i])<<8)|(((input[7]>>(4-i*2))&3)<<6));
    return (count<<16)|256;
}
extern "C" void cc_demo_update(uint8_t* state,const uint8_t* observer,uint32_t frame){
    if(state[37]==255||uint16_t(frame)<=6)return;
    w(state,38,0);
    for(unsigned i=0;i<3;++i){
        const uint32_t delta=d(observer,i*4)-d(state,1+i*4);
        const unsigned acceleration=uint16_t(delta-u(state,19+i*4));w(state,31+i*2,acceleration);
        const unsigned magnitude=absword(acceleration);
        if(magnitude>u(state,38)){w(state,38,magnitude);copy(state+40,state+19,12);}
        l(state,19+i*4,delta);
    }
}
extern "C" uint32_t cc_demo_apply(const uint8_t* state,uint8_t* observer,uint8_t* velocity,uint32_t ticksf,uint32_t ejected){
    if((state[0]&32)||uint8_t(ejected))return 0;
    copy(observer,state+1,18);
    for(unsigned i=0;i<3;++i){
        const int quotient=s(u(state,19+i*4))*s(ticksf)/10;
        if(quotient<-32768||quotient>32767)return 1;
        w(velocity,i*2,uint16_t(quotient)<<4);
    }
    return 0;
}
extern "C" uint32_t cc_demo_record(uint8_t* state,const uint8_t* observer,uint8_t* scan,uint8_t* output){
    unsigned count=0,event=0;
    if(*scan==0x96){
        *scan=0;
        if(state[37]){state[37]=0;state[0]=255;copy(output,state,37);return 37|(2u<<16);}
        state[37]=1;state[0]=11;copy(output,state,37);count=37;event=1;
    }
    if(state[37]!=1)return count|(event<<16);
    if(u(state,38)<128){cc_demo_compact(state,observer,output+count);state[0]|=8;output[count]|=8;count+=8;}
    else{state[0]&=127;state[0]|=8;copy(output+count,state,37);count+=37;}
    return count|(event<<16);
}
extern "C" void cc_demo_merge(uint8_t* state,uint32_t previous_flags){state[0]=uint8_t((state[0]|(previous_flags&52))&(previous_flags|252));}
extern "C" uint32_t cc_demo_ticks(const uint8_t* state,uint32_t normal_ticks){return state[37]==255?(state[52]?20:10):uint16_t(normal_ticks);}
