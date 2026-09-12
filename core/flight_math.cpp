#include <stdint.h>
extern "C" uint32_t cc_atn(uint32_t);
// DSQRT.ASM returns the final Newton quotient (AX), not its next guess (BX).
// Preserve its saturation, minimum one, and stopping threshold of three.
extern "C" uint32_t cc_dsqrt(uint32_t input){
    if(input&0xc0000000u)return 32767;
    uint32_t scan=input<<2;uint16_t guess=32767,alternate=23170;
    bool found=false;
    for(unsigned i=0;i<14;++i){
        bool carry=scan>>31;scan<<=1;
        if(carry){found=true;break;}
        carry=scan>>31;scan<<=1;
        if(carry){guess=alternate;found=true;break;}
        guess>>=1;alternate>>=1;
    }
    if(!found)return 1;
    for(unsigned budget=0;budget<64;++budget){
        if(!guess)return 65536u;
        uint32_t quotient=input/guess;
        if(quotient>65535)return 65536u;
        uint16_t old=guess;guess=uint16_t(uint32_t(guess)+quotient)>>1;
        int difference=int(old)-int(guess);if(difference<0)difference=-difference;
        if(difference<3)return quotient;
    }
    return 65536u;
}
// ATN.ASM atn2. Packed output bit 16 represents an x86 divide exception.
extern "C" uint32_t cc_atn2(uint32_t x,uint32_t y){
    uint16_t ax=uint16_t(x),dx=uint16_t(y),cl=0,ch=0;
    if(ax==32768)++ax;
    if(ax&32768){cl=3;ax=uint16_t(0u-ax);}
    if(dx&32768){cl^=5;dx=uint16_t(0u-dx);}
    if(ax==dx)ax=32768;
    else{
        if(uint16_t(ax-dx)&32768){ch=64;auto temp=ax;ax=dx;dx=temp;cl^=1;cl=uint16_t(((cl>>2)<<1)|(cl&1));}
        if(!ax)return 65536;
        uint32_t ratio=(uint32_t(dx)<<16)/ax;
        if(ratio>65535)return 65536;
        ax=uint16_t(cc_atn(ratio>>1));
    }
    ax>>=2;
    if(cl&1)ax=uint16_t(0u-ax);
    if(cl&2)ch=uint16_t((ch+128)&255);
    return uint16_t(uint32_t(ax)+(ch<<8));
}
extern "C" uint32_t cc_vmag(int32_t x,int32_t y,int32_t z){
    const int16_t a=int16_t(x),b=int16_t(y),c=int16_t(z);
    return cc_dsqrt(uint32_t(int32_t(a)*a)+uint32_t(int32_t(b)*b)+uint32_t(int32_t(c)*c));
}
